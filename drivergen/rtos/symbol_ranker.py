"""pipeline step - symbol candidate ranker with quota-style diversity."""

from __future__ import annotations

import logging
import re
from dataclasses import dataclass

from .config import load_thresholds
from .deep_parser import ParsedFileBundle
from .types import (
    RankedSymbolCandidate,
    RepoIndexBundle,
    SlotGoal,
    SlotPlan,
    SymbolSketch,
    TaskSpec,
)
from .slot_guard import assess_symbol_fit

logger = logging.getLogger(__name__)


# Tokenization


_TOKEN_SPLIT_RE = re.compile(r"\W+")
_PATH_TOKEN_SPLIT_RE = re.compile(r"[\W_]+")
_MIN_INTENT_TOKEN_LEN = 3
_MIN_PATH_TOKEN_LEN = 2


def _intent_tokens(query_intents: list[str]) -> set[str]:
    out: set[str] = set()
    for intent in query_intents or []:
        if not isinstance(intent, str):
            continue
        for t in _TOKEN_SPLIT_RE.split(intent.lower()):
            if len(t) >= _MIN_INTENT_TOKEN_LEN:
                out.add(t)
    return out


def _path_tokens(path: str) -> list[str]:
    return [t for t in _PATH_TOKEN_SPLIT_RE.split(path.lower()) if len(t) >= _MIN_PATH_TOKEN_LEN]


# Sketch construction


def _parsed_symbols_to_sketches(
    parsed_bundles: dict[str, ParsedFileBundle],
    repo_bundle: RepoIndexBundle,
) -> list[SymbolSketch]:
    """Flatten every parsed symbol into a :class:`SymbolSketch`."""
    root_roles = {r.root_id: r.roles for r in repo_bundle.roots}
    out: list[SymbolSketch] = []
    for b in parsed_bundles.values():
        roles = root_roles.get(b.card.root_id, frozenset())
        ptokens = _path_tokens(b.card.path)
        # Bound include_context length so massive headers don't bloat
        # cache size; the ranker only consults this shallowly.
        inc_ctx = list(b.parsed.include_graph or [])[:30]

        path_lower = (b.card.path or "").lower()
        is_header_file = path_lower.endswith((".h", ".hpp", ".hh", ".hxx"))
        for fn in b.parsed.function_declarations:
            # Skip source-private functions; keep header inline wrappers.
            if fn.is_static and not (is_header_file and fn.is_inline):
                continue
            out.append(
                SymbolSketch(
                    name=fn.name,
                    kind="function",
                    signature=fn.signature,
                    file=b.card.path,
                    root_id=b.card.root_id,
                    root_roles=roles,
                    path_tokens=ptokens,
                    file_kind=b.card.file_kind,
                    dir_role_hint=b.card.dir_role_hint,
                    include_context=inc_ctx,
                )
            )
        for m in b.parsed.macro_definitions:
            out.append(
                SymbolSketch(
                    name=m.name,
                    kind="macro",
                    signature=m.value or None,
                    file=b.card.path,
                    root_id=b.card.root_id,
                    root_roles=roles,
                    path_tokens=ptokens,
                    file_kind=b.card.file_kind,
                    dir_role_hint=b.card.dir_role_hint,
                    include_context=inc_ctx,
                )
            )
        for t in b.parsed.typedef_definitions:
            out.append(
                SymbolSketch(
                    name=t.name,
                    kind="typedef",
                    signature=t.underlying or None,
                    file=b.card.path,
                    root_id=b.card.root_id,
                    root_roles=roles,
                    path_tokens=ptokens,
                    file_kind=b.card.file_kind,
                    dir_role_hint=b.card.dir_role_hint,
                    include_context=inc_ctx,
                )
            )
        for s in b.parsed.struct_definitions:
            out.append(
                SymbolSketch(
                    name=s.name,
                    kind="struct",
                    signature=None,
                    file=b.card.path,
                    root_id=b.card.root_id,
                    root_roles=roles,
                    path_tokens=ptokens,
                    file_kind=b.card.file_kind,
                    dir_role_hint=b.card.dir_role_hint,
                    include_context=inc_ctx,
                )
            )
        for e in b.parsed.enum_definitions:
            out.append(
                SymbolSketch(
                    name=e.name,
                    kind="enum",
                    signature=None,
                    file=b.card.path,
                    root_id=b.card.root_id,
                    root_roles=roles,
                    path_tokens=ptokens,
                    file_kind=b.card.file_kind,
                    dir_role_hint=b.card.dir_role_hint,
                    include_context=inc_ctx,
                )
            )
    return out


# Private and test detection


# The private/test detector intentionally stays conservative and cross-platform.
_PRIVATE_NAME_PREFIXES = ("prv", "__", "_internal_", "_private_")


def _is_private_name(name: str) -> bool:
    n = name.lower()
    return any(n.startswith(p) for p in _PRIVATE_NAME_PREFIXES)


# Scoring


@dataclass
class _Weights:
    """Closed-over scoring constants for one ranker invocation."""

    exact_name: float
    name_token: float
    sig_token: float
    path_token: float
    root_role: float
    canonical_bus: float
    unexpected_kind_mul: float
    private_mul: float
    negative_role_pen: float


def _load_weights() -> _Weights:
    cfg = load_thresholds()
    s = cfg.get("scoring", {})
    # All exposed in thresholds.json/scoring; new keys default
    # safely so the JSON can be added to incrementally.
    pref_role_bonus = float(s.get("preferred_root_role_bonus", 1.5))
    # Default negative role penalty mirrors the preferred role bonus.
    neg_role = float(
        s.get("symbol_negative_role_penalty", pref_role_bonus * -2.0)
    )
    return _Weights(
        exact_name=float(s.get("symbol_exact_name_bonus", 12.0)),
        name_token=float(s.get("symbol_name_token_weight", 3.0)),
        sig_token=float(s.get("symbol_sig_token_weight", 2.0)),
        path_token=float(s.get("symbol_path_token_weight", 1.5)),
        root_role=pref_role_bonus * 2.0,
        canonical_bus=float(s.get("bus_keyword_weight", 2.0)),
        unexpected_kind_mul=float(s.get("symbol_unexpected_kind_multiplier", 0.5)),
        private_mul=float(s.get("symbol_private_multiplier", 0.3)),
        negative_role_pen=neg_role,
    )


def _score_symbol_for_slot(
    sym: SymbolSketch,
    slot: SlotGoal,
    tokens: set[str],
    intent_lower: set[str],
    weights: _Weights,
) -> tuple[float, list[str]]:
    """Compute (score, reasons) for one symbol against one slot."""
    score = 0.0
    reasons: list[str] = []

    name_lower = sym.name.lower()
    sig_lower = (sym.signature or "").lower()

    # 1. exact-name match (case-insensitive)
    if intent_lower and name_lower in intent_lower:
        score += weights.exact_name
        reasons.append(f"exact-name+{weights.exact_name:.1f}")
    elif tokens:
        # 2. name token match (only when no exact)
        hits = sum(1 for t in tokens if t in name_lower)
        if hits:
            bonus = weights.name_token * hits
            score += bonus
            reasons.append(f"name-tok={hits}+{bonus:.1f}")

    # 3. signature token match
    if sig_lower and tokens:
        sig_hits = sum(1 for t in tokens if t in sig_lower)
        if sig_hits:
            bonus = weights.sig_token * min(sig_hits, 4)  # cap; common types
            score += bonus
            reasons.append(f"sig-tok={sig_hits}+{bonus:.1f}")

    # 4. path token match
    if sym.path_tokens and tokens:
        path_str = " ".join(sym.path_tokens)
        path_hits = sum(1 for t in tokens if t in path_str)
        if path_hits:
            bonus = weights.path_token * min(path_hits, 4)
            score += bonus
            reasons.append(f"path-tok={path_hits}+{bonus:.1f}")

    # 5. root-role match / penalty
    if sym.root_roles:
        if slot.preferred_root_roles and any(
            r in sym.root_roles for r in slot.preferred_root_roles
        ):
            score += weights.root_role
            reasons.append(f"root-role+{weights.root_role:.1f}")
        elif slot.negative_root_roles and any(
            r in sym.root_roles for r in slot.negative_root_roles
        ):
            score += weights.negative_role_pen
            reasons.append(f"neg-role{weights.negative_role_pen:+.1f}")

    # 6. canonical_bus match against name OR file path
    if slot.canonical_bus:
        bus = slot.canonical_bus.lower()
        if bus in name_lower or bus in (sym.file or "").lower():
            score += weights.canonical_bus
            reasons.append(f"bus[{bus}]+{weights.canonical_bus:.1f}")

    if score <= 0:
        return 0.0, []

    # Apply expected-kind gating after task-package helper exclusions.
    if slot.expected_kinds:
        ek = set(slot.expected_kinds)
        # Exclude slot-source markers from parsed symbol kind checks.
        ek.discard("task_package_helper")
        if ek and sym.kind not in ek:
            score *= weights.unexpected_kind_mul
            reasons.append(f"unex-kind*{weights.unexpected_kind_mul:.1f}")

    # 8. private / test penalty
    if _is_private_name(sym.name):
        score *= weights.private_mul
        reasons.append(f"private*{weights.private_mul:.1f}")

    return score, reasons


# Diversity ranking


def _diversify(
    sorted_candidates: list[RankedSymbolCandidate],
    *,
    k: int,
    max_per_file: int = 4,
    max_per_prefix: int = 8,
    prefix_len: int = 6,
) -> list[RankedSymbolCandidate]:
    """Return the first *k* candidates that respect per-file and per-prefix quotas."""
    selected: list[RankedSymbolCandidate] = []
    by_file: dict[str, int] = {}
    by_prefix: dict[str, int] = {}
    for cand in sorted_candidates:
        if len(selected) >= k:
            break
        file_key = cand.sketch.file
        prefix = cand.sketch.name[:prefix_len].lower()
        if by_file.get(file_key, 0) >= max_per_file:
            continue
        if by_prefix.get(prefix, 0) >= max_per_prefix:
            continue
        selected.append(cand)
        by_file[file_key] = by_file.get(file_key, 0) + 1
        by_prefix[prefix] = by_prefix.get(prefix, 0) + 1
    return selected


# Public entry


def rank_symbols_for_slots(
    *,
    parsed_bundles: dict[str, ParsedFileBundle],
    slot_plan: SlotPlan,
    task_spec: TaskSpec,
    repo_bundle: RepoIndexBundle,
    max_per_slot: int | None = None,
) -> dict[str, list[RankedSymbolCandidate]]:
    """Rank every parsed symbol against every slot and return the top-``max_per_slot`` per slot, with per-file / per-prefix diversity quotas app."""
    cfg = load_thresholds()
    candidate_cfg = cfg.get("candidate_ranker", {})
    if max_per_slot is None:
        max_per_slot = int(candidate_cfg.get("max_candidate_symbols_per_slot", 30))

    weights = _load_weights()

    sketches = _parsed_symbols_to_sketches(parsed_bundles, repo_bundle)
    logger.info(
        "Symbol ranker: flattened %d sketches across %d files",
        len(sketches),
        len(parsed_bundles),
    )

    out: dict[str, list[RankedSymbolCandidate]] = {}
    n_with_match = 0
    for slot in slot_plan.slots:
        tokens = _intent_tokens(slot.query_intents)
        intent_lower = {(i or "").lower() for i in (slot.query_intents or []) if i}
        scored: list[RankedSymbolCandidate] = []
        fallback: list[RankedSymbolCandidate] = []
        for sk in sketches:
            score, reasons = _score_symbol_for_slot(
                sk, slot, tokens, intent_lower, weights
            )
            assessment = assess_symbol_fit(slot, sk)
            if assessment.hard_reject:
                continue
            if assessment.score_multiplier != 1.0:
                score *= assessment.score_multiplier
                reasons.extend(assessment.reasons)
            if score <= 0:
                # Keep fallback candidates for model-assisted binding.
                fallback.append(RankedSymbolCandidate(
                    sketch=sk, score=0.0,
                    match_reasons=["fallback-no-score"],
                ))
                continue
            scored.append(RankedSymbolCandidate(
                sketch=sk, score=score, match_reasons=reasons,
            ))
        scored.sort(key=lambda c: (-c.score, c.sketch.name, c.sketch.file))
        if not scored and fallback:
            scored = fallback[:max_per_slot]
        diversified = _diversify(scored, k=max_per_slot)
        out[slot.slot_id] = diversified
        if diversified:
            n_with_match += 1

    logger.info(
        "Symbol ranker: %d/%d slots have at least one symbol candidate "
        "(max_per_slot=%d)",
        n_with_match,
        len(out),
        max_per_slot,
    )
    return out


__all__ = [
    "rank_symbols_for_slots",
]
