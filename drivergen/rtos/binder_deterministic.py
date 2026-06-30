"""pipeline step - deterministic symbol binder."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field

from .config import load_thresholds
from .deep_parser import ParsedFileBundle
from .slot_guard import filter_eligible_candidates
from .types import (
    EvidenceSpan,
    RankedSymbolCandidate,
    SOURCE_KIND_MANIFEST_REPO,
    SlotPlan,
    SymbolBinding,
    VERIFICATION_SOURCE_DECLARED,
)

logger = logging.getLogger(__name__)


# Output


@dataclass
class BinderDecision:
    """Diagnostic record for one slot's binding decision."""

    slot_id: str
    accepted: bool
    """True iff a SymbolBinding was emitted; False = deferred to LLM."""

    reason: str
    """Free-form short reason: ``exact-name``, ``clear-margin``,
    ``unique-candidate``, ``low-score``, ``no-candidates``, ``tie``."""

    binding: SymbolBinding | None = None
    """Set iff ``accepted``; the validated SymbolBinding itself."""

    candidates_seen: int = 0
    top1_score: float = 0.0
    top2_score: float = 0.0


# Helpers


def _has_exact_name_reason(cand: RankedSymbolCandidate) -> bool:
    return any(r.startswith("exact-name") for r in cand.match_reasons)


# Path-segment heuristics for board-specific headers that generic stubs
# cannot compile.  Match only directory tokens, not arbitrary substrings.
_BSP_DIR_TOKENS = ("bsp", "boards")


def _is_bsp_header(path: str) -> bool:
    """``True`` iff the header lives under a board-specific package tree."""
    if not path:
        return False
    parts = [p.lower() for p in path.replace("\\", "/").split("/") if p]
    for tok in _BSP_DIR_TOKENS:
        if tok in parts:
            return True
    return False


def _scan_bundles_for_declaring_header(
    *,
    parsed_bundles: dict[str, ParsedFileBundle],
    symbol_name: str,
    symbol_kind: str | None,
    preferred_root_id: str | None,
) -> str | None:
    """Cross-file lookup: scan parsed *headers* for one that declares the symbol the sketch is implementing."""
    if not symbol_name:
        return None

    matches: list[tuple[int, str, str]] = []
    for fkey, bundle in parsed_bundles.items():
        path = bundle.card.path or ""
        plower = path.lower()
        if not plower.endswith((".h", ".hpp", ".hh", ".hxx")):
            continue
        parsed = bundle.parsed
        hit = False
        if symbol_kind == "function":
            for fn in parsed.function_declarations:
                if fn.name == symbol_name and not fn.is_static:
                    hit = True
                    break
        elif symbol_kind == "macro":
            for m in parsed.macro_definitions:
                if m.name == symbol_name:
                    hit = True
                    break
        elif symbol_kind == "typedef":
            for t in parsed.typedef_definitions:
                if t.name == symbol_name:
                    hit = True
                    break
        elif symbol_kind == "struct":
            for s in parsed.struct_definitions:
                if s.name == symbol_name:
                    hit = True
                    break
        elif symbol_kind == "enum":
            for e in parsed.enum_definitions:
                if e.name == symbol_name:
                    hit = True
                    break
        else:
            for fn in parsed.function_declarations:
                if fn.name == symbol_name and not fn.is_static:
                    hit = True
                    break
            if not hit:
                for m in parsed.macro_definitions:
                    if m.name == symbol_name:
                        hit = True
                        break
        if not hit:
            continue
        # Lower score prefers same-root, shallower headers.
        same_root = (
            preferred_root_id is not None
            and bundle.card.root_id == preferred_root_id
        )
        score_root = 0 if same_root else 1
        matches.append((score_root, path, fkey))

    if not matches:
        return None
    matches.sort(key=lambda x: (x[0], len(x[1]), x[1]))
    return matches[0][1]


def _header_for_sketch(
    card_path: str,
    *,
    parsed_bundles: dict[str, ParsedFileBundle] | None = None,
    root_id: str | None = None,
    symbol_name: str | None = None,
    symbol_kind: str | None = None,
) -> str | None:
    """Best-effort guess at the .h that *declares* this sketch's symbol so we can populate ``required_headers``."""
    if not card_path:
        return None
    lower = card_path.lower()
    if lower.endswith((".h", ".hpp", ".hh", ".hxx")):
        return card_path
    for s in (".c", ".cpp", ".cc", ".cxx"):
        if not lower.endswith(s):
            continue
        candidate = card_path[: -len(s)] + ".h"
        if parsed_bundles is None:
            return candidate
        if root_id and f"{root_id}::{candidate}" in parsed_bundles:
            return candidate
        for b in parsed_bundles.values():
            if b.card.path == candidate:
                return candidate
        if symbol_name:
            return _scan_bundles_for_declaring_header(
                parsed_bundles=parsed_bundles,
                symbol_name=symbol_name,
                symbol_kind=symbol_kind,
                preferred_root_id=root_id,
            )
        return None
    return None


def build_binding_from_candidate(
    *,
    slot_id: str,
    cand: RankedSymbolCandidate,
    parsed_bundles: dict[str, ParsedFileBundle],
    confidence_cap: float,
) -> SymbolBinding:
    sk = cand.sketch
    declared_in = f"{sk.root_id}::{sk.file}"

    # Recover an implementation location when a matching source file exists.
    implemented_in: str | None = None
    if sk.kind == "function" and sk.file:
        sk_lower = sk.file.lower()
        for h in (".h", ".hpp", ".hh", ".hxx"):
            if not sk_lower.endswith(h):
                continue
            counterpart_rel = sk.file[: -len(h)] + ".c"
            ckey = f"{sk.root_id}::{counterpart_rel}"
            cb = parsed_bundles.get(ckey)
            if cb and any(fn.name == sk.name for fn in cb.parsed.function_declarations):
                implemented_in = ckey
            break

    header = _header_for_sketch(
        sk.file,
        parsed_bundles=parsed_bundles,
        root_id=sk.root_id,
        symbol_name=sk.name,
        symbol_kind=sk.kind,
    )

    # Board-package headers are real matches but unsuitable for generic stubs.
    bsp_header_filtered: str | None = None
    if header and _is_bsp_header(header):
        bsp_header_filtered = header
        header = None

    required_headers = [header] if header else []

    # Exact-name matches are the strongest deterministic signal.
    if _has_exact_name_reason(cand):
        confidence = 1.0
    else:
        confidence = min(cand.score / max(confidence_cap, 1e-6), 1.0)

    evidence = [
        EvidenceSpan(
            root_id=sk.root_id,
            path=sk.file,
            kind="declaration",
        )
    ]
    if implemented_in:
        impl_root, impl_rel = implemented_in.split("::", 1)
        evidence.append(
            EvidenceSpan(root_id=impl_root, path=impl_rel, kind="implementation")
        )

    notes = list(cand.match_reasons)
    if bsp_header_filtered:
        notes.append(f"_filtered_bsp_header={bsp_header_filtered}")

    return SymbolBinding(
        slot_id=slot_id,
        symbol=sk.name,
        kind=sk.kind,
        source_kind=SOURCE_KIND_MANIFEST_REPO,
        verification=VERIFICATION_SOURCE_DECLARED,
        signature=sk.signature,
        signature_source="parser",
        declared_in=declared_in,
        implemented_in=implemented_in,
        required_headers=required_headers,
        required_types=[],  # filled by the type-graph walk
        return_semantics=None,
        semantic_role=None,
        confidence=confidence,
        allowed_for_codegen=True,
        requires_runtime_provision=False,
        evidence=evidence,
        notes=notes,
    )


# Public entry


def bind_symbols_deterministic(
    *,
    ranks: dict[str, list[RankedSymbolCandidate]],
    slot_plan: SlotPlan,
    parsed_bundles: dict[str, ParsedFileBundle],
    task_spec=None,
) -> tuple[dict[str, SymbolBinding], list[str], dict[str, BinderDecision]]:
    """Deterministically bind symbols to slots wherever the ranker top-1 is unambiguous."""
    cfg = load_thresholds()
    binder_cfg = cfg.get("binder", {})
    top1_min = float(binder_cfg.get("top1_min_score", 5.0))
    top1_top2_ratio = float(binder_cfg.get("top1_to_top2_ratio", 1.5))
    confidence_cap = float(binder_cfg.get("confidence_score_cap", 30.0))

    bindings: dict[str, SymbolBinding] = {}
    ambiguous: list[str] = []
    decisions: dict[str, BinderDecision] = {}

    for slot in slot_plan.slots:
        raw_cands = ranks.get(slot.slot_id, [])
        if not raw_cands:
            decisions[slot.slot_id] = BinderDecision(
                slot_id=slot.slot_id, accepted=False, reason="no-candidates",
                candidates_seen=0,
            )
            ambiguous.append(slot.slot_id)
            continue

        cands, _rejected = filter_eligible_candidates(slot, raw_cands)
        if not cands:
            decisions[slot.slot_id] = BinderDecision(
                slot_id=slot.slot_id,
                accepted=False,
                reason="no-eligible-candidates",
                candidates_seen=len(raw_cands),
            )
            ambiguous.append(slot.slot_id)
            continue

        top1 = cands[0]
        top2 = cands[1] if len(cands) >= 2 else None
        top1_score = top1.score
        top2_score = top2.score if top2 else 0.0

        # 1. Strong-signal: exact-name reason.
        if _has_exact_name_reason(top1):
            binding = build_binding_from_candidate(
                slot_id=slot.slot_id, cand=top1,
                parsed_bundles=parsed_bundles,
                confidence_cap=confidence_cap,
            )
            bindings[slot.slot_id] = binding
            decisions[slot.slot_id] = BinderDecision(
                slot_id=slot.slot_id, accepted=True, reason="exact-name",
                binding=binding, candidates_seen=len(cands),
                top1_score=top1_score, top2_score=top2_score,
            )
            continue

        # 2. Clear-margin: top1 high enough AND clearly ahead of top2.
        clear_margin = (
            top1_score >= top1_min
            and (top2 is None or top1_score >= top1_top2_ratio * top2_score)
        )
        if clear_margin:
            reason = "unique-candidate" if top2 is None else "clear-margin"
            binding = build_binding_from_candidate(
                slot_id=slot.slot_id, cand=top1,
                parsed_bundles=parsed_bundles,
                confidence_cap=confidence_cap,
            )
            bindings[slot.slot_id] = binding
            decisions[slot.slot_id] = BinderDecision(
                slot_id=slot.slot_id, accepted=True, reason=reason,
                binding=binding, candidates_seen=len(cands),
                top1_score=top1_score, top2_score=top2_score,
            )
            continue

        # 3. Defer.
        defer_reason = (
            "low-score"
            if top1_score < top1_min
            else "tie"  # top1 >= min but ratio gate failed
        )
        decisions[slot.slot_id] = BinderDecision(
            slot_id=slot.slot_id, accepted=False, reason=defer_reason,
            candidates_seen=len(cands),
            top1_score=top1_score, top2_score=top2_score,
        )
        ambiguous.append(slot.slot_id)

    logger.info(
        "Deterministic binder: bound %d / %d slots (deferred %d to LLM)",
        len(bindings),
        len(slot_plan.slots),
        len(ambiguous),
    )
    if ambiguous:
        logger.info(
            "  Deferred slots: %s",
            ", ".join(ambiguous[:8]) + ("…" if len(ambiguous) > 8 else ""),
        )

    return bindings, ambiguous, decisions


__all__ = [
    "BinderDecision",
    "bind_symbols_deterministic",
    "build_binding_from_candidate",
]
