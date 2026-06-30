"""pipeline step - File Selector LLM."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field

from typing import Any, Mapping, Sequence

from .config import load_thresholds
from .file_selector import FileMatch, FileSelection
from .llm_infra import BudgetTracker, call_llm_json, get_call_budget
from .types import (
    FileCard,
    SlotPlan,
    TaskSpec,
)

logger = logging.getLogger(__name__)


# Output and diagnostics


@dataclass
class FileSelectorLLMDecision:
    """Per-slot decision record produced by the LLM File Selector."""

    slot_id: str
    accepted: bool
    """True iff at least one file was added/replaced via LLM output.
    False = the deterministic selection was kept (skipped, abstain,
    invalid output, budget exhausted)."""

    reason: str
    """``llm-selected`` / ``llm-abstained`` / ``no-candidates`` /
    ``budget_exhausted`` / ``no_provider`` / ``provider_error`` /
    ``invalid-paths`` / ``not-triggered``."""

    candidates_seen: int = 0
    selected: list[dict] = field(default_factory=list)
    rejected: list[dict] = field(default_factory=list)
    need_more_search: bool = False
    raw_telemetry: dict = field(default_factory=dict)


# Prompt construction


ROLE = (
    "You select source files from a fixed candidate list that most "
    "likely declare or implement the API a driver-generation slot needs. "
    "You see candidates extracted from a real RTOS / vendor SDK tree; "
    "pick those that fit the slot's intent and abstain when none does."
)

OUTPUT_FORMAT = (
    "Output exactly 1 JSON object — no markdown fences, no prose, no "
    "trailing commas. The schema is:\n"
    "{\n"
    "  \"selected_files\":   [{\"root_id\": \"<from candidate>\", \"path\": \"<from candidate>\"}, ...],\n"
    "  \"rejected_files\":   [{\"root_id\": \"<from candidate>\", \"path\": \"<from candidate>\", \"reason\": \"<short>\"}, ...],\n"
    "  \"need_more_search\": <bool>,\n"
    "  \"reasoning\":        \"<<= 2 sentences>\"\n"
    "}\n"
    "All four keys are required; lists may be empty when the rules below "
    "say so. UTF-8 only."
)

HARD_RULES: tuple[str, ...] = (
    # Verbatim file grounding
    'Hard rule 1 — verbatim (root_id, path) pairs: Each entry of '
    '"selected_files" and "rejected_files" MUST equal a '
    '(root_id, path) pair shown in the candidate block, character for '
    'character (case-sensitive). Acceptable: a candidate displayed as '
    '"root_id=sdk_root path=drivers/bus/src/bus_driver.c" → '
    '{"root_id": "sdk_root", "path": "drivers/bus/src/bus_driver.c"}. '
    'Counter-example: dropping the suffix to '
    '"drivers/bus/src/bus_driver" '
    '(wrong — extension differs) or inventing a new path '
    '(wrong — not in the candidate block).',

    # Selection cap
    'Hard rule 2 — at most 12 selected files: '
    '"selected_files" returns between 0 and 12 entries. When more than '
    '12 candidates look plausible, keep the strongest matches and drop '
    'the rest. Counter-example: returning all 128 candidates (wrong — '
    'downstream stages cap to 12 anyway, and your extras hide which '
    'files were actually trusted).',

    # Empty-pool abstention
    'Hard rule 3 — abstain via need_more_search when the candidate '
    'block is "(no candidates from deterministic selector)": set '
    '"selected_files": [] and "need_more_search": true so the gap '
    'diagnoser can escalate. Otherwise leave "need_more_search": false. '
    'Counter-example: returning need_more_search=false with an empty '
    'pool (wrong — no follow-up round can be triggered) or '
    'need_more_search=true while also returning a non-empty selection '
    '(wrong — those signals contradict each other).',

    # Cross-target tie-break
    'Hard rule 4 — task context breaks cross-MCU ties: When several '
    'candidates fit, prefer the file whose path matches the task '
    'context block\'s rtos / board / mcu_family / integration_style. '
    'Acceptable: for "mcu_family=family_a" pick the family_a file '
    'over a family_b sibling. Counter-example: picking a nonmatching '
    'family file because the deterministic ranker scored it higher '
    '(wrong — wrong-family files break '
    'compilation downstream).',

    # Reasoning length
    'Hard rule 5 — reasoning is at most 2 sentences: "reasoning" '
    'states why the chosen files fit the slot. Counter-example: '
    'multi-paragraph chain-of-thought (wrong — the audit ledger '
    'truncates to 240 chars and longer text gets clipped mid-sentence).',
)


def build_system_prompt() -> str:
    """Assemble the full system prompt from the module-level constants.

    Pure string concatenation — deterministic, cache-safe.
    """
    return "\n\n".join((ROLE, OUTPUT_FORMAT, *HARD_RULES))


_SYSTEM_PROMPT = build_system_prompt()


# Mechanical rule checks


def detect_violations(
    payload: Mapping[str, Any] | None,
    *,
    candidate_pairs: Sequence[tuple[str, str]],
    max_selected: int = 12,
) -> list[str]:
    """Return human-readable violation messages; empty list = compliant.

    ``candidate_pairs`` is the authoritative ``(root_id, path)`` allow-list.
    """
    issues: list[str] = []
    if not isinstance(payload, Mapping):
        return ["Hard rule violation — output is not a JSON object"]

    cand_set = {
        (r, p) for r, p in candidate_pairs
        if isinstance(r, str) and isinstance(p, str)
    }

    selected = payload.get("selected_files")
    if not isinstance(selected, list):
        issues.append(
            "Hard rule 1 violation — selected_files is not a list"
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

    rejected = payload.get("rejected_files")
    if not isinstance(rejected, list):
        issues.append(
            "Hard rule 1 violation — rejected_files is not a list"
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

    if isinstance(selected, list) and len(selected) > max_selected:
        issues.append(
            "Hard rule 2 violation — selected_files has "
            f"{len(selected)} entries, max is {max_selected}"
        )

    need_more = payload.get("need_more_search")
    if not isinstance(need_more, bool):
        issues.append(
            "Hard rule 3 violation — need_more_search is not a boolean"
        )
    else:
        if not cand_set:
            if isinstance(selected, list) and selected:
                issues.append(
                    "Hard rule 3 violation — selected_files non-empty "
                    "with empty candidate block"
                )
            if not need_more:
                issues.append(
                    "Hard rule 3 violation — empty pool requires "
                    "need_more_search=true"
                )
        else:
            if need_more and isinstance(selected, list) and selected:
                issues.append(
                    "Hard rule 3 violation — need_more_search=true "
                    "contradicts non-empty selected_files"
                )

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
        "selected_files": {
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
        "rejected_files": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "root_id": {"type": "string"},
                    "path": {"type": "string"},
                    "reason": {"type": "string"},
                },
                "required": ["root_id", "path"],
            },
        },
        "need_more_search": {"type": "boolean"},
        "reasoning": {"type": "string"},
    },
    "required": [
        "selected_files",
        "rejected_files",
        "need_more_search",
        "reasoning",
    ],
    "additionalProperties": False,
}


def _render_file_card(card: FileCard, det_match: FileMatch | None) -> str:
    role = card.dir_role_hint or "(no role)"
    syms = ", ".join(card.exported_symbols[:6]) or "(no symbols)"
    bus = ", ".join(f"{k}={v}" for k, v in sorted(card.bus_hits.items())) or "-"
    runtime = ", ".join(f"{k}={v}" for k, v in sorted(card.runtime_hits.items())) or "-"
    score_text = f"score={det_match.score:.2f}" if det_match else "score=-"
    reasons_text = ""
    if det_match and det_match.reasons:
        reasons_text = "  reasons=" + ", ".join(det_match.reasons[:5])
    return (
        f"  - root_id={card.root_id} path={card.path}\n"
        f"      kind={card.file_kind} role={role} {score_text}{reasons_text}\n"
        f"      exported_symbols=[{syms}]\n"
        f"      bus_hits={bus}; runtime_hits={runtime}"
    )


def _render_user_prompt(
    *,
    slot,
    candidates: list[tuple[FileCard, FileMatch | None]],
    task_spec: TaskSpec | None,
    deterministic_selection: FileSelection,
    max_candidates: int,
) -> str:
    intent = ", ".join(slot.query_intents or []) or "(none)"
    expected = ", ".join(slot.expected_kinds or []) or "any"
    bus = slot.canonical_bus or "(unspecified)"
    pref_roles = ", ".join(slot.preferred_root_roles or []) or "any"
    neg_roles = ", ".join(slot.negative_root_roles or []) or "none"

    trimmed = candidates[:max_candidates]
    cand_block = (
        "\n".join(_render_file_card(card, match) for card, match in trimmed)
        if trimmed else "  (no candidates from deterministic selector)"
    )

    det_summary = (
        "  empty (deterministic selector produced 0 matches)"
        if deterministic_selection.is_empty
        else (
            f"  final={len(deterministic_selection.matches)} candidates, "
            f"raw_pool={len(deterministic_selection.candidate_matches or deterministic_selection.matches)}, "
            f"top score = {deterministic_selection.matches[0].score:.2f}"
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
        f"\nDeterministic selection summary:\n{det_summary}\n"
        f"\nCandidate files (top {len(trimmed)}):\n{cand_block}\n"
        f"\nPick the files most likely to host the API. Return JSON per schema."
    )


# Trigger logic


def _slot_triggers_llm(
    selection: FileSelection, max_files_per_slot: int
) -> bool:
    """Trigger when deterministic produced 0 OR its raw candidate pool
    exceeds the final per-slot cap."""
    if selection.is_empty:
        return True
    candidate_count = len(selection.candidate_matches or selection.matches)
    if candidate_count > max_files_per_slot:
        return True
    return False


def _selection_priority(selection: FileSelection) -> tuple[int, float, float, int, str]:
    """Sort triggered slots so limited LLM calls inspect the riskiest files."""
    pool = selection.candidate_matches or selection.matches
    if not pool:
        return (0, 0.0, 0.0, 0, selection.slot_id)
    top = float(pool[0].score)
    second = float(pool[1].score) if len(pool) > 1 else 0.0
    ratio = top / second if second > 0 else 999.0
    return (1, ratio, -top, -len(pool), selection.slot_id)


# Public entry


def select_files_with_llm(
    *,
    provider,
    slot_plan: SlotPlan,
    deterministic_selections: dict[str, FileSelection],
    task_spec: TaskSpec | None,
    budget: BudgetTracker,
    max_candidate_files_per_call: int | None = None,
    metadata: dict | None = None,
) -> tuple[dict[str, FileSelection], dict[str, FileSelectorLLMDecision]]:
    """Augment / replace deterministic selections with LLM-picked ones."""
    cfg = load_thresholds()
    file_cfg = cfg.get("candidate_ranker", {})
    max_files_per_slot = int(file_cfg.get("max_candidate_files_per_slot", 12))
    if max_candidate_files_per_call is None:
        max_candidate_files_per_call = int(
            file_cfg.get("llm_rerank_candidate_files_per_slot", 128)
        )
    max_candidate_files_per_call = max(max_candidate_files_per_call, max_files_per_slot)

    out_selections: dict[str, FileSelection] = {}
    decisions: dict[str, FileSelectorLLMDecision] = {}
    triggered: list[tuple[tuple[int, float, float, int, str], Any, FileSelection]] = []

    for slot in slot_plan.slots:
        det_sel = deterministic_selections.get(
            slot.slot_id,
            FileSelection(slot_id=slot.slot_id, matches=[], fallback_used=False),
        )
        out_selections[slot.slot_id] = det_sel

        if not _slot_triggers_llm(det_sel, max_files_per_slot):
            decisions[slot.slot_id] = FileSelectorLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="not-triggered",
                candidates_seen=len(det_sel.candidate_matches or det_sel.matches),
            )
            continue

        # Candidate pool for this slot — deterministic top-N pairs of
        # (card, match) so the prompt can show scores / reasons.
        triggered.append((_selection_priority(det_sel), slot, det_sel))

    triggered.sort(key=lambda item: item[0])
    call_limit = max(0, int(get_call_budget("file_selector").max_per_task))
    llm_calls_attempted = 0

    for _priority, slot, det_sel in triggered:
        raw_matches = det_sel.candidate_matches or det_sel.matches
        pool: list[tuple[FileCard, FileMatch | None]] = [
            (m.card, m) for m in raw_matches[:max_candidate_files_per_call]
        ]

        if provider is None:
            decisions[slot.slot_id] = FileSelectorLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="no_provider",
                candidates_seen=len(pool),
            )
            continue

        if llm_calls_attempted >= call_limit:
            decisions[slot.slot_id] = FileSelectorLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="stage_call_limit",
                candidates_seen=len(pool),
                raw_telemetry={
                    "skipped": True,
                    "reason": f"file_selector max_per_task={call_limit} reached",
                },
            )
            logger.info(
                "LLM file selector skipped slot=%s: stage call limit reached (%d)",
                slot.slot_id,
                call_limit,
            )
            continue

        user_prompt = _render_user_prompt(
            slot=slot,
            candidates=pool,
            task_spec=task_spec,
            deterministic_selection=det_sel,
            max_candidates=max_candidate_files_per_call,
        )
        task_name = f"file_selector_{slot.slot_id}".replace(".", "_")
        llm_calls_attempted += 1
        logger.info(
            "LLM file selector calling slot=%s candidates=%d attempt=%d/%d",
            slot.slot_id,
            len(pool),
            llm_calls_attempted,
            call_limit,
        )
        payload, telemetry = call_llm_json(
            provider=provider,
            call_kind="file_selector",
            task_name=task_name,
            schema=_OUTPUT_SCHEMA,
            system_prompt=_SYSTEM_PROMPT,
            user_prompt=user_prompt,
            budget=budget,
            metadata=metadata,
        )
        logger.info(
            "LLM file selector returned slot=%s skipped=%s reason=%s",
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
            decisions[slot.slot_id] = FileSelectorLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason=short,
                candidates_seen=len(pool),
                raw_telemetry=telemetry,
            )
            continue

        try:
            raw_selected = payload.get("selected_files") or []
            raw_rejected = payload.get("rejected_files") or []
            need_more = bool(payload.get("need_more_search"))
            reasoning = str(payload.get("reasoning") or "").strip()[:240]
            if not isinstance(raw_selected, list):
                raw_selected = []
            if not isinstance(raw_rejected, list):
                raw_rejected = []
        except (TypeError, ValueError, AttributeError):
            decisions[slot.slot_id] = FileSelectorLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="invalid-payload",
                candidates_seen=len(pool),
                raw_telemetry={**telemetry, "payload": payload},
            )
            continue

        # Hard validation: selected (root_id, path) pairs must come from
        # the candidate pool.
        cand_index: dict[tuple[str, str], FileMatch | None] = {}
        cand_card_index: dict[tuple[str, str], FileCard] = {}
        for card, match in pool:
            key = (card.root_id, card.path)
            cand_index[key] = match
            cand_card_index[key] = card

        valid_selected: list[FileCard] = []
        invalid_selected: list[dict] = []
        for entry in raw_selected:
            if not isinstance(entry, dict):
                continue
            rid = entry.get("root_id")
            pth = entry.get("path")
            if not isinstance(rid, str) or not isinstance(pth, str):
                continue
            key = (rid.strip(), pth.strip())
            card = cand_card_index.get(key)
            if card is None:
                invalid_selected.append({"root_id": key[0], "path": key[1]})
                continue
            valid_selected.append(card)

        valid_selected = valid_selected[:max_files_per_slot]
        valid_rejected = []
        for entry in raw_rejected:
            if not isinstance(entry, dict):
                continue
            rid = entry.get("root_id")
            pth = entry.get("path")
            reason_text = entry.get("reason") or ""
            if not isinstance(rid, str) or not isinstance(pth, str):
                continue
            key = (rid.strip(), pth.strip())
            if key in cand_card_index:
                valid_rejected.append({
                    "root_id": key[0],
                    "path": key[1],
                    "reason": str(reason_text)[:160],
                })

        if not valid_selected and not pool:
            decisions[slot.slot_id] = FileSelectorLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="llm-abstained",
                candidates_seen=0,
                rejected=valid_rejected,
                need_more_search=need_more or True,
                raw_telemetry={**telemetry, "reasoning": reasoning},
            )
            continue

        if not valid_selected:
            decisions[slot.slot_id] = FileSelectorLLMDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="llm-abstained" if not invalid_selected else "invalid-paths",
                candidates_seen=len(pool),
                rejected=valid_rejected,
                need_more_search=need_more,
                raw_telemetry={
                    **telemetry, "reasoning": reasoning,
                    "invalid_paths": invalid_selected[:5],
                },
            )
            continue

        # Reuse deterministic scores for LLM-selected files.
        new_matches: list[FileMatch] = []
        for card in valid_selected:
            existing = cand_index.get((card.root_id, card.path))
            score = existing.score if existing else 0.0
            reasons = list(existing.reasons) if existing else []
            reasons.append("llm-selected")
            new_matches.append(
                FileMatch(card=card, score=score, reasons=reasons)
            )

        out_selections[slot.slot_id] = FileSelection(
            slot_id=slot.slot_id,
            matches=new_matches,
            candidate_matches=list(det_sel.candidate_matches or det_sel.matches),
            fallback_used=False,
        )
        decisions[slot.slot_id] = FileSelectorLLMDecision(
            slot_id=slot.slot_id,
            accepted=True,
            reason="llm-selected",
            candidates_seen=len(pool),
            selected=[
                {"root_id": c.root_id, "path": c.path} for c in valid_selected
            ],
            rejected=valid_rejected,
            need_more_search=need_more,
            raw_telemetry={**telemetry, "reasoning": reasoning},
        )

    n_selected = sum(1 for d in decisions.values() if d.accepted)
    n_triggered = sum(
        1 for d in decisions.values() if d.reason != "not-triggered"
    )
    logger.info(
        "LLM file selector: %d/%d slots selected (%d triggered)",
        n_selected,
        len(slot_plan.slots),
        n_triggered,
    )
    return out_selections, decisions


__all__ = [
    "FileSelectorLLMDecision",
    "HARD_RULES",
    "OUTPUT_FORMAT",
    "ROLE",
    "build_system_prompt",
    "detect_violations",
    "select_files_with_llm",
]
