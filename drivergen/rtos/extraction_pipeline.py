"""pipeline step (round-0 only) - end-to-end RTOS context extraction."""

from __future__ import annotations

import json
import logging
import re
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path

from .binder_deterministic import BinderDecision, bind_symbols_deterministic
from .binder_llm import LLMBinderDecision, bind_symbols_with_llm
from .coverage import report_slot_coverage
from .deep_parser import ParsedFileBundle, parse_selected_files
from .dir_router import DirectoryRoute, route_directories_deterministic
from .dir_router_llm import (
    DirectoryRouterLLMDecision,
    route_directories_with_llm,
)
from .file_selector import FileMatch, FileSelection, select_files_deterministic
from .file_selector_llm import (
    FileSelectorLLMDecision,
    select_files_with_llm,
)
from .gap_diagnoser_llm import GapDiagnosis, diagnose_gaps_with_llm
from .include_expand import expand_includes_and_counterparts
from .llm_infra import BudgetTracker, make_budget_tracker
from .source_kind_classifier import (
    HelperClassification,
    apply_task_package_helpers,
)
from .symbol_ranker import rank_symbols_for_slots
from .types import (
    ExtractionLedger,
    RankedSymbolCandidate,
    RepoIndexBundle,
    RoundRecord,
    SlotCoverageReport,
    SlotPlan,
    SymbolBinding,
    TaskSpec,
)

logger = logging.getLogger(__name__)

_EXACT_SYMBOL_INTENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def _exact_symbol_intents_for_slot(slot) -> tuple[str, ...]:
    out: list[str] = []
    for raw in slot.query_intents or []:
        text = str(raw or "").strip()
        if not text or not _EXACT_SYMBOL_INTENT_RE.fullmatch(text):
            continue
        # Keep exact-symbol backfill limited to API-shaped identifiers.
        if "_" not in text and not any(ch.isupper() for ch in text):
            continue
        out.append(text)
    return tuple(dict.fromkeys(out))


def _card_symbol_names(card) -> set[str]:
    names = {str(sym or "") for sym in (card.exported_symbols or []) if str(sym or "")}
    for sketch in card.candidate_symbols or []:
        name = getattr(sketch, "name", "")
        if name:
            names.add(str(name))
    return names


def _exact_symbol_card_sort_key(card, slot) -> tuple[int, int, int, str]:
    preferred = set(slot.preferred_root_roles or [])
    role = str(card.dir_role_hint or "")
    preferred_rank = 0 if role in preferred else 1
    kind = str(card.file_kind or "")
    kind_rank = 0 if kind == "header" else 1 if kind == "source" else 2
    return (preferred_rank, kind_rank, len(str(card.path or "")), str(card.path or ""))


def _backfill_exact_symbol_intent_files(
    *,
    bundle: RepoIndexBundle,
    slot_plan: SlotPlan,
    selections: dict[str, FileSelection],
    max_files_per_symbol: int = 4,
) -> dict[str, FileSelection]:
    """Add files that export exact API names already present in slot intents."""
    if not bundle.file_cards:
        return selections

    cards_by_symbol: dict[str, list] = {}
    for card in bundle.file_cards:
        kind = str(card.file_kind or "")
        if kind not in {"header", "source"}:
            continue
        for name in _card_symbol_names(card):
            cards_by_symbol.setdefault(name, []).append(card)

    for slot in slot_plan.slots:
        exact_names = _exact_symbol_intents_for_slot(slot)
        if not exact_names:
            continue
        selection = selections.setdefault(
            slot.slot_id,
            FileSelection(slot_id=slot.slot_id),
        )
        seen = {(m.card.root_id, m.card.path) for m in selection.matches}
        for symbol_name in exact_names:
            candidates = list(cards_by_symbol.get(symbol_name, ()))
            candidates.sort(key=lambda card: _exact_symbol_card_sort_key(card, slot))
            added = 0
            for card in candidates:
                key = (card.root_id, card.path)
                if key in seen:
                    continue
                match = FileMatch(
                    card=card,
                    score=999.0,
                    reasons=[f"exact_symbol_intent:{symbol_name}"],
                )
                selection.matches.append(match)
                selection.candidate_matches.append(match)
                seen.add(key)
                added += 1
                if added >= max_files_per_symbol:
                    break
    return selections


@dataclass
class ExtractionResult:
    """Everything one round-0 extraction produces."""

    bindings: dict[str, SymbolBinding] = field(default_factory=dict)
    coverage: SlotCoverageReport | None = None
    ledger: ExtractionLedger = field(default_factory=ExtractionLedger)
    parsed_bundles: dict[str, ParsedFileBundle] = field(default_factory=dict)
    ranks: dict[str, list[RankedSymbolCandidate]] = field(default_factory=dict)
    routes: dict[str, DirectoryRoute] = field(default_factory=dict)
    selections: dict[str, FileSelection] = field(default_factory=dict)

    # Per-stage decision detail — non-essential to bindings but
    # useful for audit / requires_human routing.
    deterministic_decisions: dict[str, BinderDecision] = field(default_factory=dict)
    llm_decisions: dict[str, LLMBinderDecision] = field(default_factory=dict)
    dir_router_llm_decisions: dict[str, DirectoryRouterLLMDecision] = field(
        default_factory=dict
    )
    file_selector_llm_decisions: dict[str, FileSelectorLLMDecision] = field(
        default_factory=dict
    )
    helper_classifications: list[HelperClassification] = field(default_factory=list)
    gap_diagnoses: dict[str, GapDiagnosis] = field(default_factory=dict)

    budget: BudgetTracker | None = None
    timings: dict[str, float] = field(default_factory=dict)


def run_round0_extraction(
    *,
    bundle: RepoIndexBundle,
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    provider=None,
    budget: BudgetTracker | None = None,
    metadata: dict | None = None,
) -> ExtractionResult:
    """Run the full round-0 deterministic + LLM-binder + helper-classifier chain."""
    timings: dict[str, float] = {}
    if budget is None:
        budget = make_budget_tracker(mode="default")
    metadata = metadata or {}

    ledger = ExtractionLedger()

    # Deterministic directory and file selection.
    t = time.time()
    routes = route_directories_deterministic(
        bundle=bundle, slot_plan=slot_plan, task_spec=task_spec,
    )
    timings["dir_router"] = time.time() - t

    # Model-assisted directory router (only fires when a slot's
    # deterministic shortlist is empty or above the per-slot cap).
    dir_router_llm_decisions: dict[str, DirectoryRouterLLMDecision] = {}
    if provider is not None:
        t = time.time()
        routes, dir_router_llm_decisions = route_directories_with_llm(
            provider=provider,
            slot_plan=slot_plan,
            deterministic_routes=routes,
            bundle=bundle,
            task_spec=task_spec,
            budget=budget,
            metadata=metadata,
        )
        timings["dir_router_llm"] = time.time() - t

    t = time.time()
    selections = select_files_deterministic(
        bundle=bundle, slot_plan=slot_plan, task_spec=task_spec, routes=routes,
    )
    timings["file_selector"] = time.time() - t

    # Model-assisted file selector (only fires when a slot's
    # deterministic file shortlist is empty or above the per-slot cap).
    file_selector_llm_decisions: dict[str, FileSelectorLLMDecision] = {}
    if provider is not None:
        t = time.time()
        selections, file_selector_llm_decisions = select_files_with_llm(
            provider=provider,
            slot_plan=slot_plan,
            deterministic_selections=selections,
            task_spec=task_spec,
            budget=budget,
            metadata=metadata,
        )
        timings["file_selector_llm"] = time.time() - t

    t = time.time()
    selections = _backfill_exact_symbol_intent_files(
        bundle=bundle,
        slot_plan=slot_plan,
        selections=selections,
    )
    timings["exact_symbol_file_backfill"] = time.time() - t

    t = time.time()
    parsed_bundles = parse_selected_files(bundle=bundle, selections=selections)
    timings["deep_parse"] = time.time() - t

    t = time.time()
    parsed_bundles = expand_includes_and_counterparts(
        bundle=bundle, parsed_bundles=parsed_bundles,
    )
    timings["include_expand"] = time.time() - t

    t = time.time()
    ranks = rank_symbols_for_slots(
        parsed_bundles=parsed_bundles, slot_plan=slot_plan,
        task_spec=task_spec, repo_bundle=bundle,
    )
    timings["symbol_rank"] = time.time() - t

    t = time.time()
    det_bindings, deferred, det_decisions = bind_symbols_deterministic(
        ranks=ranks,
        slot_plan=slot_plan,
        parsed_bundles=parsed_bundles,
        task_spec=task_spec,
    )
    timings["binder_deterministic"] = time.time() - t

    # Capture any provider calls made by optional fallback paths.
    round0_llm_calls = {
        kind: count for kind, count in budget.calls_by_kind.items() if count > 0
    }
    round0_record = RoundRecord(
        round=0,
        deterministic=True,
        covered_slots=list(det_bindings.keys()),
        missing_slots=list(deferred),
        new_files_added=list(parsed_bundles.keys()),
        rejected_files=[],
        llm_calls=round0_llm_calls,
        token_usage={
            "input_tokens_total": budget.input_tokens_total,
            "output_tokens_total": budget.output_tokens_total,
        },
    )
    ledger.rounds.append(round0_record)

    # Optional model-assisted symbol binding.
    llm_bindings: dict[str, SymbolBinding] = {}
    still_deferred: list[str] = list(deferred)
    llm_decisions: dict[str, LLMBinderDecision] = {}

    if provider is not None and deferred:
        # Snapshot call counts before model-assisted binding.
        prev_calls_by_kind = dict(budget.calls_by_kind)
        prev_input_tokens = budget.input_tokens_total
        prev_output_tokens = budget.output_tokens_total

        t = time.time()
        llm_bindings, still_deferred, llm_decisions = bind_symbols_with_llm(
            provider=provider,
            deferred_slot_ids=deferred,
            ranks=ranks,
            slot_plan=slot_plan,
            parsed_bundles=parsed_bundles,
            budget=budget,
            task_spec=task_spec,
            metadata=metadata,
        )
        timings["binder_llm"] = time.time() - t

        # Record only calls issued by this binding pass.
        delta_calls = {
            kind: budget.calls_by_kind.get(kind, 0) - prev_calls_by_kind.get(kind, 0)
            for kind in budget.calls_by_kind
            if budget.calls_by_kind.get(kind, 0) > prev_calls_by_kind.get(kind, 0)
        }
        delta_input = budget.input_tokens_total - prev_input_tokens
        delta_output = budget.output_tokens_total - prev_output_tokens

        # Append a separate RoundRecord for the LLM phase so the
        # ledger preserves which slots were resolved how.
        ledger.rounds.append(
            RoundRecord(
                round=0,
                deterministic=False,
                covered_slots=list(llm_bindings.keys()),
                missing_slots=list(still_deferred),
                new_files_added=[],
                rejected_files=[],
                llm_calls=delta_calls or {"symbol_binder": 0},
                token_usage={
                    "input_tokens_delta": delta_input,
                    "output_tokens_delta": delta_output,
                    "input_tokens_total": budget.input_tokens_total,
                    "output_tokens_total": budget.output_tokens_total,
                },
            )
        )

    # Gap diagnosis remains advisory and never supplies concrete API bindings.
    merged_bindings = {**det_bindings, **llm_bindings}
    gap_diagnoses: dict[str, GapDiagnosis] = {}
    if provider is not None and still_deferred:
        # Seed gap diagnosis with prior advisory search terms.
        prior_search_terms: dict[str, list[str]] = {}
        for sid, dec in dir_router_llm_decisions.items():
            terms = list(dec.search_terms or [])
            if terms:
                prior_search_terms.setdefault(sid, []).extend(terms)
        # File selection currently has no per-slot search terms to merge.

        t = time.time()
        gap_diagnoses = diagnose_gaps_with_llm(
            provider=provider,
            still_unbound_slot_ids=still_deferred,
            ranks=ranks,
            slot_plan=slot_plan,
            task_spec=task_spec,
            bindings=merged_bindings,
            budget=budget,
            metadata=metadata,
            prior_search_terms=prior_search_terms,
        )
        timings["gap_diagnoser_llm"] = time.time() - t

    # Keep fixed-context helper names out of concrete API bindings.
    gap_fallback_helpers: dict[str, str] = {}

    # Task-package helper classification.
    t = time.time()
    helper_bindings, still_unbound, classifications = apply_task_package_helpers(
        bindings=merged_bindings,
        still_deferred=still_deferred,
        slot_plan=slot_plan,
        task_spec=task_spec,
        gap_fallback_helpers=gap_fallback_helpers,
    )
    timings["source_kind_classifier"] = time.time() - t

    if helper_bindings:
        ledger.rounds.append(
            RoundRecord(
                round=0,
                deterministic=True,
                covered_slots=list(helper_bindings.keys()),
                missing_slots=list(still_unbound),
                new_files_added=[],
                rejected_files=[],
                llm_calls={},
                token_usage={},
            )
        )

    final_bindings = {**merged_bindings, **helper_bindings}

    # Coverage report
    coverage = report_slot_coverage(
        slot_plan=slot_plan, bindings=final_bindings, ranks=ranks,
    )

    logger.info(
        "Extraction complete: %d / %d slots bound "
        "(det=%d + llm=%d + helper=%d), missing_required=%d, "
        "missing_optional=%d, ambiguous=%d",
        len(final_bindings),
        len(slot_plan.slots),
        len(det_bindings),
        len(llm_bindings),
        len(helper_bindings),
        len(coverage.missing_required),
        len(coverage.missing_optional),
        len(coverage.ambiguous),
    )

    return ExtractionResult(
        bindings=final_bindings,
        coverage=coverage,
        ledger=ledger,
        parsed_bundles=parsed_bundles,
        ranks=ranks,
        routes=routes,
        selections=selections,
        deterministic_decisions=det_decisions,
        llm_decisions=llm_decisions,
        dir_router_llm_decisions=dir_router_llm_decisions,
        file_selector_llm_decisions=file_selector_llm_decisions,
        helper_classifications=classifications,
        gap_diagnoses=gap_diagnoses,
        budget=budget,
        timings=timings,
    )


def save_extraction_ledger(
    result: ExtractionResult,
    output_dir: Path,
    *,
    filename: str = "rtos_extraction_ledger.json",
) -> Path:
    """pipeline step - write ``result.ledger`` (and a small summary) to disk."""
    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / filename
    payload = {
        "ledger": asdict(result.ledger),
        "summary": {
            "n_bindings": len(result.bindings),
            "coverage": (
                {
                    "covered_required": list(result.coverage.covered_required),
                    "covered_optional": list(result.coverage.covered_optional),
                    "missing_required": list(result.coverage.missing_required),
                    "missing_optional": list(result.coverage.missing_optional),
                    "ambiguous": list(result.coverage.ambiguous),
                }
                if result.coverage is not None
                else None
            ),
            "timings_ms": {
                k: round(v * 1000.0, 2) for k, v in (result.timings or {}).items()
            },
            "budget": result.budget.to_dict() if result.budget else None,
            "deterministic_decisions": {
                slot: {
                    "accepted": d.accepted,
                    "reason": d.reason,
                    "candidates_seen": d.candidates_seen,
                    "top1_score": d.top1_score,
                    "top2_score": d.top2_score,
                }
                for slot, d in result.deterministic_decisions.items()
            },
            "llm_decisions": {
                slot: {
                    "accepted": d.accepted,
                    "reason": d.reason,
                    "chosen_name": d.chosen_name,
                    "chosen_kind": d.chosen_kind,
                    "confidence": d.confidence,
                }
                for slot, d in result.llm_decisions.items()
            },
            "dir_router_llm_decisions": {
                slot: {
                    "accepted": d.accepted,
                    "reason": d.reason,
                    "candidates_seen": d.candidates_seen,
                    "selected_paths": list(d.selected_paths),
                    "rejected_paths": list(d.rejected_paths),
                    "search_terms": list(d.search_terms),
                }
                for slot, d in result.dir_router_llm_decisions.items()
                if d.reason != "not-triggered"
            },
            "file_selector_llm_decisions": {
                slot: {
                    "accepted": d.accepted,
                    "reason": d.reason,
                    "candidates_seen": d.candidates_seen,
                    "n_selected": len(d.selected),
                    "n_rejected": len(d.rejected),
                    "need_more_search": d.need_more_search,
                }
                for slot, d in result.file_selector_llm_decisions.items()
                if d.reason != "not-triggered"
            },
            "helper_classifications": [
                {
                    "slot_id": h.slot_id,
                    "accepted": h.accepted,
                    "helper_name": h.helper_name,
                    "reason": h.reason,
                }
                for h in result.helper_classifications
            ],
            "gap_diagnoses": {
                slot: {
                    "requires_human": d.requires_human,
                    "suggested_search_terms": list(d.suggested_search_terms),
                    "fallback_helper_name": d.fallback_helper_name,
                    "reasoning": d.reasoning,
                    "skipped_reason": d.skipped_reason,
                }
                for slot, d in result.gap_diagnoses.items()
            },
        },
    }
    out_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    logger.info("Wrote extraction ledger to %s (%d KB)",
                out_path, out_path.stat().st_size // 1024)
    return out_path


__all__ = [
    "ExtractionResult",
    "run_round0_extraction",
    "save_extraction_ledger",
]
