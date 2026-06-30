"""pipeline step - symbol binder."""

from __future__ import annotations

import json
import logging
import re
from dataclasses import dataclass, field
from typing import Any, Iterable, Mapping

from .binder_deterministic import build_binding_from_candidate
from .config import load_thresholds
from .deep_parser import ParsedFileBundle
from .slot_guard import (
    assess_symbol_fit,
    filter_eligible_candidates,
    kind_allowed_for_slot,
    slot_policy_prompt,
)
from .llm_infra import (
    BudgetTracker,
    call_llm_json,
)
from .types import (
    RankedSymbolCandidate,
    SlotPlan,
    SymbolBinding,
)

logger = logging.getLogger(__name__)


# Output and diagnostics


@dataclass
class LLMBinderDecision:
    """Per-slot decision record produced by the symbol binder."""

    slot_id: str
    accepted: bool
    """True iff a SymbolBinding was emitted by this stage."""

    reason: str
    """``llm-bound`` / ``llm-abstained`` / ``no-candidates`` /
    ``budget_exhausted`` / ``no_provider`` / ``provider_error`` /
    ``invalid-symbol`` / ``invalid-kind``."""

    binding: SymbolBinding | None = None
    candidates_seen: int = 0
    chosen_name: str | None = None
    chosen_kind: str | None = None
    confidence: float = 0.0
    raw_telemetry: dict = field(default_factory=dict)


# Prompt construction


ROLE = (
    "You bind a driver-generation slot to one API symbol from a fixed "
    "ranked candidate list. Each candidate is a real symbol parsed from "
    "an RTOS / vendor SDK source tree. Choose the best fit or abstain — "
    "you never invent identifiers."
)

OUTPUT_FORMAT = (
    "Output exactly 1 JSON object — no markdown fences, no prose, no "
    "trailing commas. The schema is:\n"
    "{\n"
    "  \"decision\":   \"bind\" | \"abstain\",\n"
    "  \"symbol\":     \"<exact name from candidates>\" | null,\n"
    "  \"kind\":       \"function\" | \"macro\" | \"typedef\" | \"struct\" | \"enum\" | null,\n"
    "  \"confidence\": <number in [0.0, 1.0]>,\n"
    "  \"reasoning\":  \"<<= 2 sentences>\"\n"
    "}\n"
    "All five keys are required. UTF-8 only. When decision = \"bind\", "
    "symbol and kind are non-null; when decision = \"abstain\" both are "
    "null."
)

ALLOWED_KINDS: frozenset[str] = frozenset(
    {"function", "macro", "typedef", "struct", "enum"}
)

HARD_RULES: tuple[str, ...] = (
    # Verbatim symbol grounding
    'Hard rule 1 — verbatim candidate names: When decision = "bind", '
    '"symbol" MUST equal the "name=" value of one candidate, character '
    'for character (case-sensitive). Acceptable: a candidate displayed '
    'as "name=bus_transfer, kind=function" → '
    '{"symbol": "bus_transfer", "kind": "function"}. '
    'Counter-example: shortening to "transfer" '
    '(wrong — name differs) or returning a name absent from the '
    'candidate list (wrong — would yield invalid-symbol).',

    # Kind agreement
    'Hard rule 2 — kind matches candidate kind: When decision = "bind" '
    'and a kind is supplied, "kind" MUST equal the candidate\'s parsed '
    'kind from one of '
    '{function, macro, typedef, struct, enum}; it also MUST be allowed '
    'by the slot\'s expected kinds listed in the prompt. Counter-example: '
    'reporting kind="function" for a candidate whose parsed kind was '
    '"macro" (wrong — drives wrong call-site generation downstream), '
    'or binding a function to a "*.key_types" slot that '
    'expects struct/typedef/enum/macro-style evidence (wrong — this '
    'turns an API call into a type contract).',

    # Cross-target tie-break
    'Hard rule 3 — task context pins the right MCU family: When several '
    'candidates look semantically equivalent, prefer the one whose '
    '"file=" path lives under the task context\'s '
    'rtos / board / mcu_family / integration_style. When every '
    'candidate is from a different MCU family, abstain with '
    'reasoning starting "wrong-mcu-family" so the gap diagnoser can '
    'flag a missing search root rather than the binder silently '
    'binding the wrong vendor. Acceptable: for "mcu_family=family_a", '
    'pick the family_a candidate over family_b or family_c siblings. '
    'Counter-example: binding a nonmatching sibling when no matching '
    'candidate is in the list (wrong — abstain instead).',

    # Public API preference
    'Hard rule 4 — prefer the RTOS HAL over vendor internals: When '
    'candidates with similar scores include both RTOS-native functions '
    '(using the RTOS\'s own types and conventions) and vendor MCU '
    'functions (declared extern in .c files, using vendor-specific '
    'handle types), pick the RTOS-native one. '
    'Vendor functions are acceptable only when no native candidate exists. '
    'Native indicators: source tag public_hal_api, signature uses RTOS '
    'types, or declared in a header under include/ / inc/. Vendor '
    'indicators: source tag mcu_internal, extern keyword in signature, '
    'uses MCU-vendor types.',

    # Abstention policy
    'Hard rule 5 — abstain when no candidate fits: If every candidate '
    'name would require renaming, mismatches the slot\'s intent, or '
    'fails the cross-MCU check, or comes only from docs/examples/'
    'templates rather than authoritative API sources, return '
    'decision="abstain" with '
    '"symbol": null and "kind": null. The unbound slot reaches the '
    'gap diagnoser instead of producing a bad binding. '
    'Integration binding slots should '
    'prefer task/board context values over arbitrary repo symbols and '
    'must not be satisfied by functions. '
    'Counter-example: forcing decision="bind" with the highest-scoring '
    'candidate even when its semantics differ (wrong — silently binds '
    'the wrong API and surfaces only at compile / runtime).',

    # Confidence scale
    'Hard rule 6 — confidence in [0.0, 1.0]: "confidence" reflects how '
    'certain you are the chosen candidate solves the slot. 0.9+ means '
    'name + signature + file path all agree; 0.5–0.8 means a partial '
    'match or the chosen top of a near-tie; <0.5 with '
    'decision="abstain" is the safe choice when nothing fits at all. '
    'Counter-example: returning confidence=0.99 alongside '
    'decision="abstain" (wrong — confidence claims a binding that '
    'doesn\'t exist).',

    # Near-tie behavior
    'Hard rule 7 — a deterministic near-tie does not require abstention: '
    'when the top 2 candidates have very close scores (within roughly 5%) '
    'AND the top candidate\'s file path matches the task context '
    '(rtos / board / mcu_family / integration_style) AND its name shares '
    'a primary topic token with the slot id (e.g. slot "i2c.read" → '
    'candidate name contains "i2c"), return decision="bind" with '
    'confidence in 0.5-0.7 rather than abstaining. Apply Rule 4 first to '
    'prefer public HAL over MCU internals within the near-tie. '
    'Acceptable: slot '
    '"i2c.read" with two same-score in-context candidates → bind '
    'the context-matching top candidate with confidence=0.6. Counter-example: '
    'abstaining whenever any tie exists, even when a clear task-context '
    'match is present (wrong — pushes the slot to gap diagnoser / human '
    'review unnecessarily and breaks slot_coverage downstream).',

    # Reasoning length
    'Hard rule 8 — reasoning is at most 2 sentences: "reasoning" '
    'briefly justifies the bind / abstain choice. Counter-example: '
    'multi-paragraph chain-of-thought (wrong — the audit ledger '
    'truncates to 240 chars and longer text gets clipped mid-sentence).',

    # ioctl command macros are valid bindings.
    'Hard rule 9 - ioctl command macros are valid bus-operation bindings: '
    'Some codebases expose bus operations via ioctl() calls with named command '
    'macros. These macros are the correct API surface when they define the '
    'operation the driver calls via ioctl(fd, COMMAND, arg). Do not abstain '
    'just because a candidate is a macro constant rather than a callable '
    'function. If the candidate name clearly matches the slot operation, bind '
    'it with confidence 0.6-0.8.',
)


def build_system_prompt() -> str:
    """Assemble the binder system prompt from the module-level constants."""
    return "\n\n".join((ROLE, OUTPUT_FORMAT, *HARD_RULES))


_SYSTEM_PROMPT = build_system_prompt()


# Mechanical rule checks


def _source_layer_note(file_path: str, symbol_name: str = "") -> str:
    """Return a short source-layer tag for one candidate file path."""
    p = file_path.lower().replace("\\", "/")
    # Public API: declarations from include/ headers
    if re.search(r"/(?:include|inc)/", p) and p.endswith(".h"):
        return ", public_hal_api"
    # Public API headers in top-level interface directories.
    if p.endswith(".h") and not re.search(r"/(?:mcu|arch|vendor)/", p):
        return ", public_hal_api"
    # Vendor implementation sources in low-level trees.
    if re.search(r"/(?:mcu|arch|vendor)/", p):
        return ", mcu_internal"
    # Implementation source files (not headers): less authoritative than public header declarations
    if p.endswith(".c"):
        return ", implementation"
    return ""


def _candidate_names(
    candidates: Iterable[RankedSymbolCandidate],
) -> dict[str, str]:
    """Return ``{name: kind}`` for the supplied ranked candidates."""
    out: dict[str, str] = {}
    for c in candidates:
        if c is None:
            continue
        sk = getattr(c, "sketch", None)
        if sk is None:
            continue
        name = getattr(sk, "name", None)
        kind = getattr(sk, "kind", None)
        if isinstance(name, str) and name and isinstance(kind, str):
            out[name] = kind
    return out


def detect_violations(
    payload: Mapping[str, Any] | None,
    *,
    candidates: Iterable[RankedSymbolCandidate],
    slot=None,
) -> list[str]:
    """Return human-readable violation messages; empty list = compliant."""
    issues: list[str] = []
    if not isinstance(payload, Mapping):
        return ["Hard rule violation — output is not a JSON object"]

    name_to_kind = _candidate_names(candidates)

    decision = payload.get("decision")
    if decision not in {"bind", "abstain"}:
        issues.append(
            f"Hard rule violation — decision must be 'bind' or 'abstain', got {decision!r}"
        )

    symbol = payload.get("symbol")
    kind = payload.get("kind")

    if decision == "bind":
        if not isinstance(symbol, str) or not symbol.strip():
            issues.append(
                "Hard rule 1 violation — bind requires a non-empty symbol string"
            )
        elif symbol not in name_to_kind:
            issues.append(
                f"Hard rule 1 violation — symbol not in candidates: {symbol!r}"
            )
        elif kind is not None:
            if not isinstance(kind, str) or kind not in ALLOWED_KINDS:
                issues.append(
                    f"Hard rule 2 violation — kind not in allowed set: {kind!r}"
                )
            elif name_to_kind[symbol] != kind:
                issues.append(
                    "Hard rule 2 violation — kind disagrees with candidate "
                    f"({kind!r} vs {name_to_kind[symbol]!r})"
                )
            elif not kind_allowed_for_slot(slot, kind):
                issues.append(
                    f"Hard rule 2b violation — kind {kind!r} is not eligible for slot"
                )
    elif decision == "abstain":
        if symbol is not None:
            issues.append(
                "Hard rule 4 violation — abstain requires symbol=null"
            )
        if kind is not None:
            issues.append(
                "Hard rule 4 violation — abstain requires kind=null"
            )

    confidence = payload.get("confidence")
    try:
        cval = float(confidence)
    except (TypeError, ValueError):
        cval = None
    if cval is None:
        issues.append("Hard rule 5 violation — confidence is not a number")
    elif not (0.0 <= cval <= 1.0):
        issues.append(
            f"Hard rule 5 violation — confidence {cval} outside [0.0, 1.0]"
        )

    reasoning = payload.get("reasoning")
    if not isinstance(reasoning, str):
        issues.append("Hard rule 7 violation — reasoning is not a string")
    elif reasoning.count(".") > 4 or len(reasoning) > 480:
        issues.append(
            "Hard rule 7 violation — reasoning exceeds the 2-sentence budget"
        )

    return issues


_OUTPUT_SCHEMA = {
    "type": "object",
    "properties": {
        "decision": {"type": "string", "enum": ["bind", "abstain"]},
        "symbol": {"type": ["string", "null"]},
        "kind": {"type": ["string", "null"]},
        "confidence": {"type": "number"},
        "reasoning": {"type": "string"},
    },
    "required": ["decision", "symbol", "kind", "confidence", "reasoning"],
    "additionalProperties": False,
}


def _render_user_prompt(
    *,
    slot,
    candidates: list[RankedSymbolCandidate],
    max_candidates: int,
    task_spec=None,
) -> str:
    """Build the user-side prompt for one slot."""
    trimmed = candidates[:max_candidates]
    rendered_lines: list[str] = []
    for i, c in enumerate(trimmed, start=1):
        sk = c.sketch
        sig = (sk.signature or "").strip()
        # Cap signature to keep prompt short on type-heavy candidates.
        if len(sig) > 200:
            sig = sig[:200] + "…"
        reasons = ", ".join(c.match_reasons[:5])
        fit = assess_symbol_fit(slot, c).prompt_note
        source_note = _source_layer_note(sk.file or "", sk.name or "")
        rendered_lines.append(
            f"Candidate {i}: name={sk.name}, kind={sk.kind}, file={sk.file}, "
            f"score={c.score:.2f}, reasons=[{reasons}]\n"
            f"  source_fit: {fit}{source_note}\n"
            f"  signature: {sig if sig else '(no signature)'}"
        )
    candidate_block = "\n".join(rendered_lines)

    intent = ", ".join(slot.query_intents or []) or "(none)"
    expected = ", ".join(slot.expected_kinds or []) or "any"
    bus = slot.canonical_bus or "(unspecified)"
    pref_roles = ", ".join(slot.preferred_root_roles or []) or "any"

    task_block = ""
    if task_spec is not None:
        task_block = (
            f"\nTask context (use this to break MCU / board ties):\n"
            f"  rtos: {task_spec.rtos_id or '(none)'}\n"
            f"  board: {task_spec.board or '(none)'}\n"
            f"  mcu_family: {task_spec.mcu_family or '(none)'}\n"
            f"  integration_style: {task_spec.integration_style or '(none)'}\n"
        )

    return (
        f"Slot id: {slot.slot_id}\n"
        f"Slot layer: {slot.layer}\n"
        f"Required: {slot.required}\n"
        f"Intent phrases: {intent}\n"
        f"Expected symbol kinds: {expected}\n"
        f"Canonical bus: {bus}\n"
        f"Preferred root roles: {pref_roles}\n"
        f"Slot semantic policy:\n{slot_policy_prompt(slot)}\n"
        f"{task_block}"
        f"\n"
        f"Candidates:\n{candidate_block}\n"
        f"\n"
        f"Pick the best candidate (or abstain).  Return JSON per schema."
    )


# LLM output validation


def _find_candidate(
    candidates: list[RankedSymbolCandidate],
    name: str,
    kind: str | None,
) -> RankedSymbolCandidate | None:
    """Return the first candidate whose ``(name, kind)`` matches."""
    for c in candidates:
        if c.sketch.name != name:
            continue
        if kind is not None and c.sketch.kind != kind:
            continue
        return c
    return None


# Public entry


def bind_symbols_with_llm(
    *,
    provider,
    deferred_slot_ids: list[str],
    ranks: dict[str, list[RankedSymbolCandidate]],
    slot_plan: SlotPlan,
    parsed_bundles: dict[str, ParsedFileBundle],
    budget: BudgetTracker,
    task_spec=None,
    max_candidates_per_slot: int | None = None,
    metadata: dict | None = None,
) -> tuple[dict[str, SymbolBinding], list[str], dict[str, LLMBinderDecision]]:
    """Run the symbol binder over each deferred slot."""
    cfg = load_thresholds()
    cand_cfg = cfg.get("candidate_ranker", {})
    if max_candidates_per_slot is None:
        max_candidates_per_slot = int(cand_cfg.get("llm_binder_topk", 15))

    binder_cfg = cfg.get("binder", {})
    confidence_cap = float(binder_cfg.get("confidence_score_cap", 30.0))

    bindings: dict[str, SymbolBinding] = {}
    still_deferred: list[str] = []
    decisions: dict[str, LLMBinderDecision] = {}

    plan_by_id = {s.slot_id: s for s in slot_plan.slots}

    for slot_id in deferred_slot_ids:
        slot = plan_by_id.get(slot_id)
        raw_candidates = ranks.get(slot_id, [])
        if slot is None:
            decisions[slot_id] = LLMBinderDecision(
                slot_id=slot_id, accepted=False,
                reason="no-slot-in-plan", candidates_seen=len(raw_candidates),
            )
            still_deferred.append(slot_id)
            continue
        if not raw_candidates:
            decisions[slot_id] = LLMBinderDecision(
                slot_id=slot_id, accepted=False,
                reason="no-candidates", candidates_seen=0,
            )
            still_deferred.append(slot_id)
            continue

        candidates, rejected_candidates = filter_eligible_candidates(slot, raw_candidates)
        if not candidates:
            decisions[slot_id] = LLMBinderDecision(
                slot_id=slot_id,
                accepted=False,
                reason="no-eligible-candidates",
                candidates_seen=len(raw_candidates),
                raw_telemetry={
                    "rejected": [
                        {
                            "name": c.sketch.name,
                            "kind": c.sketch.kind,
                            "file": c.sketch.file,
                            "reasons": list(a.reasons),
                            "tags": sorted(a.tags),
                        }
                        for c, a in rejected_candidates[:10]
                    ]
                },
            )
            still_deferred.append(slot_id)
            continue

        user_prompt = _render_user_prompt(
            slot=slot,
            candidates=candidates,
            max_candidates=max_candidates_per_slot,
            task_spec=task_spec,
        )
        task_name = f"symbol_binder_{slot_id}".replace(".", "_")
        payload, telemetry = call_llm_json(
            provider=provider,
            call_kind="symbol_binder",
            task_name=task_name,
            schema=_OUTPUT_SCHEMA,
            system_prompt=_SYSTEM_PROMPT,
            user_prompt=user_prompt,
            budget=budget,
            metadata=metadata,
        )

        if payload is None:
            reason = telemetry.get("reason", "unknown")
            if "budget" in reason:
                short_reason = "budget_exhausted"
            elif reason == "no_provider":
                short_reason = "no_provider"
            else:
                short_reason = "provider_error"
            decisions[slot_id] = LLMBinderDecision(
                slot_id=slot_id, accepted=False,
                reason=short_reason, candidates_seen=len(candidates),
                raw_telemetry=telemetry,
            )
            still_deferred.append(slot_id)
            continue

        decision = (payload.get("decision") or "").lower()
        if decision != "bind":
            decisions[slot_id] = LLMBinderDecision(
                slot_id=slot_id, accepted=False,
                reason="llm-abstained", candidates_seen=len(candidates),
                confidence=float(payload.get("confidence") or 0.0),
                raw_telemetry={**telemetry, "reasoning": payload.get("reasoning")},
            )
            still_deferred.append(slot_id)
            continue

        chosen_name = payload.get("symbol")
        chosen_kind = payload.get("kind")
        if not isinstance(chosen_name, str) or not chosen_name.strip():
            decisions[slot_id] = LLMBinderDecision(
                slot_id=slot_id, accepted=False,
                reason="invalid-symbol", candidates_seen=len(candidates),
                raw_telemetry={**telemetry, "payload": payload},
            )
            still_deferred.append(slot_id)
            continue

        chosen_name = chosen_name.strip()
        chosen_kind_str = (
            chosen_kind.strip() if isinstance(chosen_kind, str) and chosen_kind.strip() else None
        )

        # Validate by candidate name, allowing kind repair from parser data.
        cand = _find_candidate(candidates, chosen_name, chosen_kind_str)
        if cand is None and chosen_kind_str is not None:
            cand = _find_candidate(candidates, chosen_name, None)
        if cand is None:
            decisions[slot_id] = LLMBinderDecision(
                slot_id=slot_id, accepted=False,
                reason="invalid-symbol",
                candidates_seen=len(candidates),
                chosen_name=chosen_name,
                chosen_kind=chosen_kind_str,
                raw_telemetry={**telemetry, "payload": payload},
            )
            still_deferred.append(slot_id)
            continue

        # Parser kind is authoritative.
        binding = build_binding_from_candidate(
            slot_id=slot_id,
            cand=cand,
            parsed_bundles=parsed_bundles,
            confidence_cap=confidence_cap,
        )
        # Clamp LLM confidence; malformed values keep the deterministic score.
        try:
            raw_conf = payload.get("confidence")
            if raw_conf is None:
                llm_conf = binding.confidence
            else:
                llm_conf = max(0.0, min(1.0, float(raw_conf)))
        except (TypeError, ValueError):
            llm_conf = binding.confidence
        binding.confidence = llm_conf
        # Annotate the binding so audit / ledger can see it came via LLM.
        notes = list(binding.notes)
        notes.append(f"llm_bound:{(payload.get('reasoning') or '').strip()[:120]}")
        binding.notes = notes

        bindings[slot_id] = binding
        decisions[slot_id] = LLMBinderDecision(
            slot_id=slot_id, accepted=True,
            reason="llm-bound", binding=binding,
            candidates_seen=len(candidates),
            chosen_name=cand.sketch.name,
            chosen_kind=cand.sketch.kind,
            confidence=llm_conf,
            raw_telemetry={**telemetry, "reasoning": payload.get("reasoning")},
        )

    n_bound = len(bindings)
    n_input = len(deferred_slot_ids)
    logger.info(
        "LLM symbol binder: bound %d / %d deferred slots (still deferred: %d)",
        n_bound,
        n_input,
        len(still_deferred),
    )
    return bindings, still_deferred, decisions


__all__ = [
    "ALLOWED_KINDS",
    "HARD_RULES",
    "LLMBinderDecision",
    "OUTPUT_FORMAT",
    "ROLE",
    "bind_symbols_with_llm",
    "build_system_prompt",
    "detect_violations",
]
