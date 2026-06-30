"""pipeline step - Directory Router LLM."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from typing import Any, Mapping, Sequence

from .config import load_thresholds
from .dir_router import DirectoryMatch, DirectoryRoute
from .llm_infra import BudgetTracker, call_llm_json, get_call_budget
from .types import (
    DirectoryCard,
    RepoIndexBundle,
    SlotPlan,
    TaskSpec,
)

logger = logging.getLogger(__name__)


# Output and diagnostics


@dataclass
class DirectoryRouterLLMDecision:
    """Per-slot decision record produced by the LLM Directory Router."""

    slot_id: str
    accepted: bool
    """True iff at least one directory was added/replaced via LLM
    output.  False = the deterministic route was kept (skipped, abstain,
    invalid output, budget exhausted)."""

    reason: str
    """Free-form short reason: ``llm-routed`` / ``llm-abstained`` /
    ``no-candidates`` / ``budget_exhausted`` / ``no_provider`` /
    ``provider_error`` / ``invalid-paths`` / ``not-triggered``."""

    candidates_seen: int = 0
    selected_paths: list[dict] = field(default_factory=list)
    """``[{"root_id": ..., "path": ...}, ...]`` — composite key.

    The field name is kept as ``selected_paths`` for ledger backwards
    compatibility, but the entries are now objects (not bare strings)
    because two roots inside the same bundle can have identical
    ``dir_path``s — see Hard rule 1 / detect_violations for the
    grounding rule the LLM follows."""

    rejected_paths: list[dict] = field(default_factory=list)
    """Same shape as :attr:`selected_paths`."""

    search_terms: list[str] = field(default_factory=list)
    raw_telemetry: dict = field(default_factory=dict)


# Prompt construction


ROLE = (
    "You route a driver-generation slot to the right directories in a "
    "real RTOS / vendor SDK source tree. You see a fixed candidate list; "
    "pick the directories that best fit the slot's intent and abstain "
    "when none does."
)

OUTPUT_FORMAT = (
    "Output exactly 1 JSON object — no markdown fences, no prose, no "
    "trailing commas. The schema is:\n"
    "{\n"
    "  \"selected_directories\": [{\"root_id\": \"<from candidate>\", \"path\": \"<from candidate>\"}, ...],\n"
    "  \"rejected_directories\": [{\"root_id\": \"<from candidate>\", \"path\": \"<from candidate>\"}, ...],\n"
    "  \"search_terms\":          [\"<short token>\", ...],\n"
    "  \"reasoning\":              \"<<= 2 sentences>\"\n"
    "}\n"
    "All four keys are required. Lists may be empty when the rules below "
    "say so. UTF-8 only.\n"
    "Each directory entry MUST be an OBJECT carrying both root_id and "
    "path because two roots in the bundle can have the same dir_path "
    "(for example, two SDK roots can both contain ``drivers/bus/include``); "
    "the pair (root_id, path) is the only unambiguous key."
)

HARD_RULES: tuple[str, ...] = (
    # Verbatim directory grounding
    'Hard rule 1 — verbatim candidate (root_id, path) pairs: Every entry '
    'of "selected_directories" and "rejected_directories" MUST be an '
    'OBJECT whose root_id and path values come verbatim from the same '
    'candidate block line, character for character (case-sensitive). '
    'Acceptable: a candidate displayed as '
    '"root_id=sdk_root:driver_framework:1 path=drivers/bus/src" → '
    '{"root_id": "sdk_root:driver_framework:1", '
    '"path": "drivers/bus/src"}. Counter-examples: '
    'returning a bare path string (wrong — must be object); '
    'rewriting "Src" to "src/" (wrong — case + trailing slash differ); '
    'mixing root_id from one candidate with path from another (wrong — '
    'breaks the (root_id, path) pair invariant).',

    # Selection cap
    'Hard rule 2 — at most 8 selected paths: '
    '"selected_directories" returns between 0 and 8 entries. When more '
    'than 8 candidates look plausible, keep the strongest matches and '
    'drop the rest. Counter-example: returning all 64 candidates when '
    'only 3 actually match the slot intent (wrong — downstream stages '
    'cap to 8 anyway and your extras get dropped silently, hurting the '
    'audit trail).',

    # Empty-pool abstention
    'Hard rule 3 — abstain with search_terms when the candidate block '
    'reads "(no candidates from deterministic router)": '
    'set "selected_directories": [] and emit between 3 and 8 '
    '"search_terms". Each search_term is a real RTOS / vendor / API '
    'token visible elsewhere in the user prompt (slot intent, expected '
    'kinds, task context). Counter-example: returning '
    '"search_terms": [] when the pool is empty (wrong — no follow-up '
    'round can be triggered) or using made-up tokens like "i2c_init_2" '
    '(wrong — must be groundable in the visible context).',

    # Cross-target tie-break
    'Hard rule 4 — task context breaks cross-MCU ties: When several '
    'candidates fit, prefer the one whose path matches the task '
    'context block\'s rtos / board / mcu_family / integration_style. '
    'Acceptable: for "mcu_family=family_a" choose the matching '
    'family_a path over a family_b sibling. Counter-example: picking '
    'a nonmatching family path because it has a higher score in the '
    'deterministic ranker (wrong — '
    'cross-MCU mismatch breaks compilation).',

    # Reasoning length
    'Hard rule 5 — reasoning is at most 2 sentences: "reasoning" '
    'states why the chosen paths fit the slot. Counter-example: '
    'multi-paragraph chain-of-thought (wrong — the audit ledger '
    'truncates to 240 chars and longer text gets clipped mid-sentence).',
)


def build_system_prompt() -> str:
    """Assemble the full system prompt from ROLE / OUTPUT_FORMAT / HARD_RULES."""
    return "\n\n".join((ROLE, OUTPUT_FORMAT, *HARD_RULES))


# Cached so callers can reuse the same string object.  The body is
# deterministic, so the cache never needs invalidation.
_SYSTEM_PROMPT = build_system_prompt()


# Mechanical rule checks


def detect_violations(
    payload: Mapping[str, Any] | None,
    *,
    candidate_pairs: Sequence[tuple[str, str]],
    max_selected: int = 8,
) -> list[str]:
    """Return human-readable violation messages; empty list = compliant."""
    issues: list[str] = []
    if not isinstance(payload, Mapping):
        return ["Hard rule violation — output is not a JSON object"]

    cand_set = {
        (r, p) for r, p in candidate_pairs
        if isinstance(r, str) and isinstance(p, str)
    }

    # Check selected_directories grounding.
    selected = payload.get("selected_directories")
    if not isinstance(selected, list):
        issues.append(
            "Hard rule 1 violation — selected_directories is not a list"
        )
        selected = []
    for entry in selected:
        if not isinstance(entry, Mapping):
            issues.append(
                f"Hard rule 1 violation — selected entry is not an object: {entry!r}"
            )
            continue
        rid = entry.get("root_id")
        pth = entry.get("path")
        if not isinstance(rid, str) or not isinstance(pth, str):
            issues.append(
                "Hard rule 1 violation — selected entry missing string root_id/path"
            )
            continue
        if (rid, pth) not in cand_set:
            issues.append(
                f"Hard rule 1 violation — selected pair not in candidates: ({rid!r}, {pth!r})"
            )

    rejected = payload.get("rejected_directories")
    if not isinstance(rejected, list):
        issues.append(
            "Hard rule 1 violation — rejected_directories is not a list"
        )
        rejected = []
    for entry in rejected:
        if not isinstance(entry, Mapping):
            continue
        rid = entry.get("root_id")
        pth = entry.get("path")
        if not isinstance(rid, str) or not isinstance(pth, str):
            issues.append(
                "Hard rule 1 violation — rejected entry missing string root_id/path"
            )
            continue
        if (rid, pth) not in cand_set:
            issues.append(
                f"Hard rule 1 violation — rejected pair not in candidates: ({rid!r}, {pth!r})"
            )

    # Check selection cap.
    if isinstance(selected, list) and len(selected) > max_selected:
        issues.append(
            "Hard rule 2 violation — selected_directories has "
            f"{len(selected)} entries, max is {max_selected}"
        )

    # Check empty-pool abstention.
    search_terms = payload.get("search_terms")
    if not isinstance(search_terms, list):
        issues.append(
            "Hard rule 3 violation — search_terms is not a list"
        )
        search_terms = []
    if not cand_set:
        if isinstance(selected, list) and selected:
            issues.append(
                "Hard rule 3 violation — selected_directories is non-empty "
                "but the candidate block was empty"
            )
        valid_terms = [t for t in search_terms if isinstance(t, str) and t.strip()]
        if not (3 <= len(valid_terms) <= 8):
            issues.append(
                "Hard rule 3 violation — empty pool requires 3..8 "
                f"search_terms, got {len(valid_terms)}"
            )

    # Check reasoning length.
    reasoning = payload.get("reasoning")
    if not isinstance(reasoning, str):
        issues.append("Hard rule 5 violation — reasoning is not a string")
    elif reasoning.count(".") > 4 or len(reasoning) > 480:
        issues.append(
            "Hard rule 5 violation — reasoning exceeds the 2-sentence budget"
        )

    return issues


_OUTPUT_SCHEMA = {
    "type": "object",
    "properties": {
        "selected_directories": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "root_id": {"type": "string"},
                    "path": {"type": "string"},
                },
                "required": ["root_id", "path"],
            },
        },
        "rejected_directories": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "root_id": {"type": "string"},
                    "path": {"type": "string"},
                },
                "required": ["root_id", "path"],
            },
        },
        "search_terms": {
            "type": "array",
            "items": {"type": "string"},
        },
        "reasoning": {"type": "string"},
    },
    "required": [
        "selected_directories",
        "rejected_directories",
        "search_terms",
        "reasoning",
    ],
    "additionalProperties": False,
}


def _render_dir_card(card: DirectoryCard) -> str:
    bus = ", ".join(f"{k}={v}" for k, v in sorted(card.bus_hits.items())) or "-"
    runtime = ", ".join(f"{k}={v}" for k, v in sorted(card.runtime_hits.items())) or "-"
    board = ", ".join(f"{k}={v}" for k, v in sorted(card.board_hits.items())) or "-"
    role = card.role_hint or "(no role)"
    syms = ", ".join(card.top_symbols[:6]) or "(no symbols)"
    return (
        f"  - root_id={card.root_id} path={card.dir_path}\n"
        f"      role_hint={role} files={card.file_count} (code={card.code_file_count})\n"
        f"      bus_hits={bus}; runtime_hits={runtime}; board_hits={board}\n"
        f"      top_symbols=[{syms}]"
    )


def _render_user_prompt(
    *,
    slot,
    candidates: list[DirectoryCard],
    task_spec: TaskSpec | None,
    deterministic_route: DirectoryRoute,
    max_candidates: int,
) -> str:
    intent = ", ".join(slot.query_intents or []) or "(none)"
    expected = ", ".join(slot.expected_kinds or []) or "any"
    bus = slot.canonical_bus or "(unspecified)"
    pref_roles = ", ".join(slot.preferred_root_roles or []) or "any"
    neg_roles = ", ".join(slot.negative_root_roles or []) or "none"

    trimmed = candidates[:max_candidates]
    cand_block = (
        "\n".join(_render_dir_card(c) for c in trimmed)
        if trimmed else "  (no candidates from deterministic router)"
    )

    det_summary = (
        f"  empty (deterministic router produced 0 matches)"
        if deterministic_route.is_empty
        else (
            f"  final={len(deterministic_route.matches)} candidates, "
            f"raw_pool={len(deterministic_route.candidate_matches or deterministic_route.matches)}, "
            f"top score = {deterministic_route.matches[0].score:.2f}"
        )
    )

    task_block = ""
    if task_spec is not None:
        task_block = (
            f"\nTask context (use to break ties on cross-MCU / cross-board):\n"
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
        f"Negative root roles: {neg_roles}\n"
        f"{task_block}"
        f"\nDeterministic route summary:\n{det_summary}\n"
        f"\nCandidate directories (top {len(trimmed)}):\n{cand_block}\n"
        f"\nPick the directories most likely to host the API. Return JSON per schema."
    )


# Trigger logic


def _slot_triggers_llm(
    route: DirectoryRoute, max_dirs_per_slot: int
) -> bool:
    """True when the deterministic route is empty OR its raw candidate pool has more candidates than the final per-slot cap."""
    if route.is_empty:
        return True
    candidate_count = len(route.candidate_matches or route.matches)
    if candidate_count > max_dirs_per_slot:
        return True
    return False


def _route_priority(route: DirectoryRoute) -> tuple[int, float, float, int, str]:
    """Sort triggered slots so scarce LLM calls cover the riskiest slots first."""
    pool = route.candidate_matches or route.matches
    if not pool:
        return (0, 0.0, 0.0, 0, route.slot_id)
    top = float(pool[0].score)
    second = float(pool[1].score) if len(pool) > 1 else 0.0
    ratio = top / second if second > 0 else 999.0
    return (1, ratio, -top, -len(pool), route.slot_id)


def _gather_candidate_pool(
    bundle: RepoIndexBundle,
    deterministic_route: DirectoryRoute,
    pool_cap: int,
) -> list[DirectoryCard]:
    """Build the candidate list shown to the LLM."""
    if deterministic_route.is_empty:
        return []
    raw_matches = deterministic_route.candidate_matches or deterministic_route.matches
    return [m.card for m in raw_matches[:pool_cap]]


# Public entry


def route_directories_with_llm(
    *,
    provider,
    slot_plan: SlotPlan,
    deterministic_routes: dict[str, DirectoryRoute],
    bundle: RepoIndexBundle,
    task_spec: TaskSpec | None,
    budget: BudgetTracker,
    max_candidate_dirs_per_call: int | None = None,
    metadata: dict | None = None,
) -> tuple[dict[str, DirectoryRoute], dict[str, DirectoryRouterLLMDecision]]:
    """Augment / replace deterministic routes with LLM-routed ones."""
    cfg = load_thresholds()
    scoring_cfg = cfg.get("scoring", {})
    max_dirs_per_slot = int(scoring_cfg.get("max_dirs_per_slot", 8))
    if max_candidate_dirs_per_call is None:
        max_candidate_dirs_per_call = int(
            scoring_cfg.get("llm_rerank_candidate_dirs_per_slot", 64)
        )
    max_candidate_dirs_per_call = max(max_candidate_dirs_per_call, max_dirs_per_slot)

    out_routes: dict[str, DirectoryRoute] = {}
    decisions: dict[str, DirectoryRouterLLMDecision] = {}
    triggered: list[tuple[tuple[int, float, float, int, str], Any, DirectoryRoute]] = []

    for slot in slot_plan.slots:
        det_route = deterministic_routes.get(
            slot.slot_id,
            DirectoryRoute(slot_id=slot.slot_id, matches=[]),
        )
        # Default: keep the deterministic route.
        out_routes[slot.slot_id] = det_route

        if not _slot_triggers_llm(det_route, max_dirs_per_slot):
            decisions[slot.slot_id] = DirectoryRouterLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="not-triggered",
                candidates_seen=len(det_route.candidate_matches or det_route.matches),
            )
            continue

        triggered.append((_route_priority(det_route), slot, det_route))

    triggered.sort(key=lambda item: item[0])
    call_limit = max(0, int(get_call_budget("directory_router").max_per_task))
    llm_calls_attempted = 0

    for _priority, slot, det_route in triggered:
        candidates = _gather_candidate_pool(bundle, det_route, max_candidate_dirs_per_call)

        if provider is None:
            decisions[slot.slot_id] = DirectoryRouterLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="no_provider",
                candidates_seen=len(candidates),
            )
            continue

        if llm_calls_attempted >= call_limit:
            decisions[slot.slot_id] = DirectoryRouterLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="stage_call_limit",
                candidates_seen=len(candidates),
                raw_telemetry={
                    "skipped": True,
                    "reason": f"directory_router max_per_task={call_limit} reached",
                },
            )
            logger.info(
                "LLM dir router skipped slot=%s: stage call limit reached (%d)",
                slot.slot_id,
                call_limit,
            )
            continue

        user_prompt = _render_user_prompt(
            slot=slot,
            candidates=candidates,
            task_spec=task_spec,
            deterministic_route=det_route,
            max_candidates=max_candidate_dirs_per_call,
        )
        task_name = f"directory_router_{slot.slot_id}".replace(".", "_")
        llm_calls_attempted += 1
        logger.info(
            "LLM dir router calling slot=%s candidates=%d attempt=%d/%d",
            slot.slot_id,
            len(candidates),
            llm_calls_attempted,
            call_limit,
        )
        payload, telemetry = call_llm_json(
            provider=provider,
            call_kind="directory_router",
            task_name=task_name,
            schema=_OUTPUT_SCHEMA,
            system_prompt=_SYSTEM_PROMPT,
            user_prompt=user_prompt,
            budget=budget,
            metadata=metadata,
        )
        logger.info(
            "LLM dir router returned slot=%s skipped=%s reason=%s",
            slot.slot_id,
            telemetry.get("skipped"),
            telemetry.get("reason", ""),
        )

        if payload is None:
            reason = telemetry.get("reason", "unknown")
            short = (
                "no_provider" if reason == "no_provider"
                else "budget_exhausted" if "budget" in reason
                else "provider_error"
            )
            decisions[slot.slot_id] = DirectoryRouterLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason=short,
                candidates_seen=len(candidates),
                raw_telemetry=telemetry,
            )
            continue

        # Defensive parsing — provider doesn't enforce strict schema.
        # Narrow except so KeyboardInterrupt / MemoryError surface.
        try:
            raw_selected = payload.get("selected_directories") or []
            raw_rejected = payload.get("rejected_directories") or []
            raw_terms = payload.get("search_terms") or []
            reasoning = str(payload.get("reasoning") or "").strip()[:240]
            if not isinstance(raw_selected, list):
                raw_selected = []
            if not isinstance(raw_rejected, list):
                raw_rejected = []
            if not isinstance(raw_terms, list):
                raw_terms = []
        except (TypeError, ValueError, AttributeError):
            decisions[slot.slot_id] = DirectoryRouterLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="invalid-payload",
                candidates_seen=len(candidates),
                raw_telemetry={**telemetry, "payload": payload},
            )
            continue

        # Validate selected composite keys against the candidate pool.
        cand_index: dict[tuple[str, str], DirectoryCard] = {}
        for c in candidates:
            cand_index[(c.root_id, c.dir_path)] = c

        valid_selected: list[DirectoryCard] = []
        invalid_selected: list[dict] = []
        for entry in raw_selected:
            if not isinstance(entry, dict):
                continue
            rid = entry.get("root_id")
            pth = entry.get("path")
            if not isinstance(rid, str) or not isinstance(pth, str):
                continue
            key = (rid.strip(), pth.strip())
            card = cand_index.get(key)
            if card is None:
                invalid_selected.append({"root_id": key[0], "path": key[1]})
                continue
            valid_selected.append(card)

        # Cap to the per-slot scoring limit so we don't blow the file
        # selector pool downstream.
        valid_selected = valid_selected[:max_dirs_per_slot]
        valid_terms = [
            str(t).strip() for t in raw_terms
            if isinstance(t, (str, int, float)) and str(t).strip()
        ][:8]
        valid_rejected: list[dict] = []
        for entry in raw_rejected:
            if not isinstance(entry, dict):
                continue
            rid = entry.get("root_id")
            pth = entry.get("path")
            if not isinstance(rid, str) or not isinstance(pth, str):
                continue
            key = (rid.strip(), pth.strip())
            if key in cand_index:
                valid_rejected.append({"root_id": key[0], "path": key[1]})

        if not valid_selected and not candidates:
            # Empty deterministic + LLM also abstained — keep the empty
            # route, but record the search_terms for the gap diagnoser.
            decisions[slot.slot_id] = DirectoryRouterLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="llm-abstained",
                candidates_seen=0,
                search_terms=valid_terms,
                raw_telemetry={**telemetry, "reasoning": reasoning},
            )
            continue

        if not valid_selected:
            # Preserve deterministic fallback when the LLM abstains.
            decisions[slot.slot_id] = DirectoryRouterLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="llm-abstained" if not invalid_selected else "invalid-paths",
                candidates_seen=len(candidates),
                rejected_paths=valid_rejected,
                search_terms=valid_terms,
                raw_telemetry={
                    **telemetry, "reasoning": reasoning,
                    "invalid_paths": invalid_selected[:5],
                },
            )
            continue

        # Reuse deterministic scores for LLM-selected directories.
        det_score_lookup = {
            (m.card.root_id, m.card.dir_path): m.score
            for m in (det_route.candidate_matches or det_route.matches)
        }
        det_reasons_lookup = {
            (m.card.root_id, m.card.dir_path): m.reasons
            for m in (det_route.candidate_matches or det_route.matches)
        }
        new_matches: list[DirectoryMatch] = []
        for card in valid_selected:
            key = (card.root_id, card.dir_path)
            score = det_score_lookup.get(key, 0.0)
            reasons = list(det_reasons_lookup.get(key, []))
            reasons.append("llm-routed")
            new_matches.append(
                DirectoryMatch(card=card, score=score, reasons=reasons)
            )

        out_routes[slot.slot_id] = DirectoryRoute(
            slot_id=slot.slot_id,
            matches=new_matches,
            candidate_matches=list(det_route.candidate_matches or det_route.matches),
        )
        decisions[slot.slot_id] = DirectoryRouterLLMDecision(
            slot_id=slot.slot_id,
            accepted=True,
            reason="llm-routed",
            candidates_seen=len(candidates),
            selected_paths=[
                {"root_id": c.root_id, "path": c.dir_path} for c in valid_selected
            ],
            rejected_paths=valid_rejected,
            search_terms=valid_terms,
            raw_telemetry={**telemetry, "reasoning": reasoning},
        )

    n_routed = sum(1 for d in decisions.values() if d.accepted)
    n_triggered = sum(
        1 for d in decisions.values()
        if d.reason not in ("not-triggered",)
    )
    logger.info(
        "LLM dir router: %d/%d slots routed (%d triggered, %d skipped)",
        n_routed,
        len(slot_plan.slots),
        n_triggered,
        len(slot_plan.slots) - n_triggered,
    )
    return out_routes, decisions


__all__ = [
    "DirectoryRouterLLMDecision",
    "HARD_RULES",
    "OUTPUT_FORMAT",
    "ROLE",
    "build_system_prompt",
    "detect_violations",
    "route_directories_with_llm",
]
