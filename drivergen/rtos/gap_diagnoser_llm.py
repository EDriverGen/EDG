"""pipeline step - Gap Diagnoser LLM."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from typing import Any, Iterable, Mapping

from .llm_infra import BudgetTracker, call_llm_json
from .types import (
    RankedSymbolCandidate,
    SlotPlan,
    SymbolBinding,
    TaskSpec,
)

logger = logging.getLogger(__name__)


@dataclass
class GapDiagnosis:
    """generated triage record for an unbound slot."""

    slot_id: str
    requires_human: bool
    suggested_search_terms: list[str] = field(default_factory=list)
    fallback_helper_name: str | None = None
    reasoning: str = ""
    raw_telemetry: dict = field(default_factory=dict)
    skipped_reason: str | None = None
    """``None`` when the LLM produced a structured answer; otherwise
    the short reason the call was skipped (``no_provider`` /
    ``budget_exhausted`` / ``provider_error`` / ``invalid_payload``)."""


_OUTPUT_SCHEMA = {
    "type": "object",
    "properties": {
        "requires_human": {"type": "boolean"},
        "suggested_search_terms": {
            "type": "array",
            "items": {"type": "string"},
        },
        "fallback_helper_name": {"type": ["string", "null"]},
        "reasoning": {"type": "string"},
    },
    "required": [
        "requires_human",
        "suggested_search_terms",
        "fallback_helper_name",
        "reasoning",
    ],
    "additionalProperties": False,
}


ROLE = (
    "You triage a driver-generation slot that the deterministic and LLM "
    "binders both could not resolve. You do not bind anything — instead "
    "you produce an advisory diagnosis: should a human review the slot, "
    "and which extra search terms might surface the missing API?"
)


OUTPUT_FORMAT = (
    "Output exactly 1 JSON object — no markdown fences, no prose, no "
    "trailing commas. The schema is:\n"
    "{\n"
    "  \"requires_human\":          <true|false>,\n"
    "  \"suggested_search_terms\":  [<3 to 8 strings>],\n"
    "  \"fallback_helper_name\":    null,\n"
    "  \"reasoning\":               \"<<= 2 sentences>\"\n"
    "}\n"
    "All four keys are required. UTF-8 only."
)


HARD_RULES: tuple[str, ...] = (
    # Search terms must come from the observed ecosystem
    'Hard rule 1 — grounded search terms: every entry in '
    '"suggested_search_terms" MUST be a substring or vendor prefix '
    'observable in either the slot\'s ranked candidates or the '
    'sibling bindings on the task (e.g. "HAL_I2C_", "hpl_i2c", "esp_i2c"). '
    'Acceptable: with sibling bindings showing HAL_I2C_Master_Transmit, '
    'returning ["HAL_I2C_Master_Receive_IT", "HAL_I2C_Mem_Read"]. '
    'Counter-example: returning ["i2c_async_v3"] when no sibling/candidate '
    'mentions that string (wrong — invents an API namespace).',

    # Fixed-context helper fallback is disabled
    'Hard rule 2 — fallback_helper_name MUST be null. Fixed-context '
    'helper hints must not be used as a shortcut for RTOS API extraction.',

    # Search-term count
    'Hard rule 3 — 3 to 8 search terms: "suggested_search_terms" has at '
    'least 3 and at most 8 entries, each non-empty and unique. The lower '
    'bound forces real diagnosis; the upper bound keeps the audit ledger '
    'short. Counter-example: 1 term (wrong — too thin to retry retrieval) '
    'or 12 terms (wrong — exceeds budget, gets truncated mid-string).',

    # Legal output states
    'Hard rule 4 — only these two states are legal: '
    '(false, null) when ranked candidates look plausible and a later '
    'retrieval/binding round should retry; or (true, null) when no '
    'candidate looks usable and human review is needed.',

    # Reasoning length
    'Hard rule 5 — reasoning is at most 2 sentences: "reasoning" briefly '
    'justifies the requires_human / fallback decision. Counter-example: '
    'multi-paragraph chain-of-thought (wrong — the audit ledger '
    'truncates to 240 chars and longer text gets clipped mid-sentence).',
)


def build_system_prompt() -> str:
    """Assemble the gap-diagnoser system prompt from the module-level pieces."""
    return "\n\n".join((ROLE, OUTPUT_FORMAT, *HARD_RULES))


_SYSTEM_PROMPT = build_system_prompt()


# Mechanical rule checks


def _grounded_term_pool(
    candidates: Iterable[RankedSymbolCandidate],
    sibling_bindings: Mapping[str, SymbolBinding] | None,
) -> list[str]:
    """Return a list of strings the diagnosis is allowed to draw search terms from."""
    pool: list[str] = []
    for c in candidates or []:
        sk = getattr(c, "sketch", None)
        if sk is None:
            continue
        name = getattr(sk, "name", None)
        if isinstance(name, str) and name:
            pool.append(name)
        file = getattr(sk, "file", None)
        if isinstance(file, str) and file:
            pool.append(file)
    if sibling_bindings:
        for b in sibling_bindings.values():
            sym = getattr(b, "symbol", None)
            if isinstance(sym, str) and sym:
                pool.append(sym)
    return pool


def _split_tokens(s: str) -> set[str]:
    """Split an identifier or path into lowercase tokens of length >= 2."""
    out: set[str] = set()
    if not isinstance(s, str) or not s:
        return out
    buf: list[str] = []
    for ch in s:
        if ch.isalnum():
            buf.append(ch.lower())
        else:
            if buf:
                tok = "".join(buf)
                if len(tok) >= 2:
                    out.add(tok)
                buf = []
    if buf:
        tok = "".join(buf)
        if len(tok) >= 2:
            out.add(tok)
    return out


def _term_is_grounded(term: str, pool_tokens: set[str]) -> bool:
    """A term is grounded when it shares >=1 token of length >=2 with the pool."""
    return bool(_split_tokens(term) & pool_tokens)


_GENERIC_HELPER_TOKENS: frozenset[str] = frozenset(
    {
        # cross-cutting shared tokens that should NOT count as topic agreement
        "rt", "thread", "rtos", "task", "app", "user", "lib", "common",
        "util", "utils", "helper", "helpers", "hook", "hooks", "fn", "func",
    }
)


def detect_violations(
    payload: Mapping[str, Any] | None,
    *,
    candidates: Iterable[RankedSymbolCandidate] = (),
    sibling_bindings: Mapping[str, SymbolBinding] | None = None,
    slot_id: str | None = None,
) -> list[str]:
    """Return human-readable violation messages; empty list = compliant."""
    issues: list[str] = []
    if not isinstance(payload, Mapping):
        return ["Hard rule violation — output is not a JSON object"]

    pool = _grounded_term_pool(candidates, sibling_bindings)

    fallback = payload.get("fallback_helper_name")
    if fallback is not None:
        issues.append(
            "Hard rule 2 violation — fallback_helper_name must be null; "
            "fixed-context helpers are not valid extraction shortcuts"
        )

    terms = payload.get("suggested_search_terms")
    if not isinstance(terms, list):
        issues.append("Hard rule 3 violation — suggested_search_terms must be a list")
        terms_list: list[str] = []
    else:
        terms_list = [str(t) for t in terms if isinstance(t, (str, int, float))]
        if len(terms_list) < 3:
            issues.append(
                f"Hard rule 3 violation — need >=3 search terms, got {len(terms_list)}"
            )
        elif len(terms_list) > 8:
            issues.append(
                f"Hard rule 3 violation — at most 8 search terms, got {len(terms_list)}"
            )
        if len({t.strip().lower() for t in terms_list}) != len(terms_list):
            issues.append("Hard rule 3 violation — duplicate search terms detected")

    pool_tokens: set[str] = set()
    for entry in pool:
        pool_tokens |= _split_tokens(entry)
    if pool_tokens:
        ungrounded = [t for t in terms_list if not _term_is_grounded(t, pool_tokens)]
        if ungrounded:
            issues.append(
                "Hard rule 1 violation — search terms not grounded in candidates / "
                f"sibling bindings: {ungrounded!r}"
            )

    requires_human = payload.get("requires_human")
    if not isinstance(requires_human, bool):
        issues.append(
            f"Hard rule violation — requires_human must be a bool, got {requires_human!r}"
        )

    reasoning = payload.get("reasoning")
    if not isinstance(reasoning, str):
        issues.append("Hard rule 5 violation — reasoning is not a string")
    elif reasoning.count(".") > 4 or len(reasoning) > 480:
        issues.append(
            "Hard rule 5 violation — reasoning exceeds the 2-sentence budget"
        )

    return issues


def _render_user_prompt(
    *,
    slot,
    candidates: list[RankedSymbolCandidate],
    task_spec: TaskSpec,
    other_bindings: dict[str, SymbolBinding],
    prior_search_terms: list[str] | None = None,
    max_candidates: int = 10,
) -> str:
    """User prompt with slot + ranked candidates + sibling bindings."""
    intent = ", ".join(slot.query_intents or []) or "(none)"
    expected = ", ".join(slot.expected_kinds or []) or "any"
    bus = slot.canonical_bus or "(unspecified)"

    cand_lines: list[str] = []
    for i, c in enumerate(candidates[:max_candidates], start=1):
        sk = c.sketch
        cand_lines.append(
            f"  {i}. {sk.name} (kind={sk.kind}, file={sk.file}, "
            f"score={c.score:.2f})"
        )
    cand_block = "\n".join(cand_lines) if cand_lines else "  (no candidates seen)"

    sibling_lines: list[str] = []
    for sid, b in sorted(other_bindings.items())[:10]:
        sibling_lines.append(f"  {sid} -> {b.symbol} ({b.source_kind})")
    sibling_block = "\n".join(sibling_lines) if sibling_lines else "  (none)"

    prior_block = ""
    if prior_search_terms:
        cleaned = [
            t for t in (s.strip() for s in prior_search_terms if isinstance(s, str))
            if t
        ][:10]
        if cleaned:
            prior_block = (
                f"\nPrior search-term hints from earlier LLM stages "
                f"(directory router / file selector) on the same deferred "
                f"chain:\n  {', '.join(cleaned)}\n"
                f"You may keep, refine, or replace these — but every "
                f"final ``suggested_search_terms`` entry MUST satisfy "
                f"Hard rule 1 (grounded in candidates / sibling bindings).\n"
            )

    return (
        f"Slot id: {slot.slot_id}\n"
        f"Required: {slot.required}\n"
        f"Layer: {slot.layer}\n"
        f"Canonical bus: {bus}\n"
        f"Intent phrases: {intent}\n"
        f"Expected symbol kinds: {expected}\n"
        f"\n"
        f"Fixed-context helper fallback is disabled; diagnose from "
        f"ranked RTOS candidates and sibling bindings only.\n"
        f"\n"
        f"Sibling bindings already chosen on this task:\n{sibling_block}\n"
        f"\n"
        f"Candidates ranked for this slot (deterministic top-{max_candidates}):\n{cand_block}\n"
        f"{prior_block}"
        f"\nDiagnose. Return JSON per schema."
    )


def diagnose_gaps_with_llm(
    *,
    provider,
    still_unbound_slot_ids: list[str],
    ranks: dict[str, list[RankedSymbolCandidate]],
    slot_plan: SlotPlan,
    task_spec: TaskSpec,
    bindings: dict[str, SymbolBinding],
    budget: BudgetTracker,
    metadata: dict | None = None,
    max_slots: int = 5,
    prior_search_terms: dict[str, list[str]] | None = None,
) -> dict[str, GapDiagnosis]:
    """Produce a :class:`GapDiagnosis` for every still-unbound slot."""
    plan_by_id = {s.slot_id: s for s in slot_plan.slots}
    out: dict[str, GapDiagnosis] = {}
    prior_terms_map = prior_search_terms or {}

    n_called = 0
    for slot_id in still_unbound_slot_ids:
        slot = plan_by_id.get(slot_id)
        if slot is None:
            out[slot_id] = GapDiagnosis(
                slot_id=slot_id, requires_human=True,
                reasoning="slot id not in plan", skipped_reason="no_slot_in_plan",
            )
            continue

        if n_called >= max_slots:
            out[slot_id] = GapDiagnosis(
                slot_id=slot_id, requires_human=True,
                reasoning="gap diagnoser per-task ceiling reached",
                skipped_reason="max_slots_per_task",
            )
            continue

        cands = ranks.get(slot_id, [])
        user_prompt = _render_user_prompt(
            slot=slot,
            candidates=cands,
            task_spec=task_spec,
            other_bindings=bindings,
            prior_search_terms=prior_terms_map.get(slot_id),
        )

        task_name = f"gap_diagnoser_{slot_id}".replace(".", "_")
        payload, telemetry = call_llm_json(
            provider=provider,
            call_kind="gap_diagnoser",
            task_name=task_name,
            schema=_OUTPUT_SCHEMA,
            system_prompt=_SYSTEM_PROMPT,
            user_prompt=user_prompt,
            budget=budget,
            metadata=metadata,
        )
        n_called += 1

        if payload is None:
            reason = telemetry.get("reason", "unknown")
            short_reason = (
                "no_provider" if reason == "no_provider"
                else "budget_exhausted" if "budget" in reason
                else "provider_error"
            )
            out[slot_id] = GapDiagnosis(
                slot_id=slot_id, requires_human=True,
                reasoning=f"gap diagnoser unavailable ({short_reason})",
                skipped_reason=short_reason,
                raw_telemetry=telemetry,
            )
            continue

        # Handle missing optional fields while preserving fatal errors.
        try:
            requires_human = bool(payload.get("requires_human"))
            terms = payload.get("suggested_search_terms") or []
            if not isinstance(terms, list):
                terms = []
            terms = [str(t) for t in terms if isinstance(t, (str, int, float))]
            fallback = payload.get("fallback_helper_name")
            if isinstance(fallback, str) and fallback.strip():
                fallback = fallback.strip()
            else:
                fallback = None
            reasoning = str(payload.get("reasoning") or "").strip()[:240]
        except (TypeError, ValueError, AttributeError) as exc:
            out[slot_id] = GapDiagnosis(
                slot_id=slot_id, requires_human=True,
                reasoning=f"invalid LLM payload: {exc}",
                skipped_reason="invalid_payload",
                raw_telemetry={**telemetry, "payload": payload},
            )
            continue

        out[slot_id] = GapDiagnosis(
            slot_id=slot_id,
            requires_human=requires_human,
            suggested_search_terms=terms[:8],  # cap to 8
            fallback_helper_name=fallback,
            reasoning=reasoning,
            raw_telemetry=telemetry,
        )

    logger.info(
        "Gap diagnoser: produced %d diagnoses (%d called, %d skipped)",
        len(out),
        n_called,
        sum(1 for d in out.values() if d.skipped_reason is not None),
    )
    return out


__all__ = [
    "GapDiagnosis",
    "HARD_RULES",
    "OUTPUT_FORMAT",
    "ROLE",
    "build_system_prompt",
    "detect_violations",
    "diagnose_gaps_with_llm",
]
