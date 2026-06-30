"""pipeline step - deterministic directory router."""

from __future__ import annotations

import logging
import re
from dataclasses import dataclass, field

from .config import load_thresholds
from .types import (
    DirectoryCard,
    RepoIndexBundle,
    SlotGoal,
    SlotPlan,
    TaskSpec,
)

logger = logging.getLogger(__name__)


# Output dataclasses


@dataclass
class DirectoryMatch:
    """One scored directory candidate for a slot."""

    card: DirectoryCard
    score: float
    reasons: list[str] = field(default_factory=list)
    """Free-form score-contribution log; empty when score == 0.  Used
    for diagnostic dumps and the LLM Directory Router prompt in
    directory routing."""


@dataclass
class DirectoryRoute:
    """One slot's directory shortlist (sorted by score, desc)."""

    slot_id: str
    matches: list[DirectoryMatch] = field(default_factory=list)
    candidate_matches: list[DirectoryMatch] = field(default_factory=list)

    @property
    def top_match(self) -> DirectoryMatch | None:
        return self.matches[0] if self.matches else None

    @property
    def is_empty(self) -> bool:
        return not self.matches


# Helpers


_TOKEN_SPLIT_RE = re.compile(r"\W+")
_MIN_TOKEN_LEN = 3


def _intent_tokens(query_intents: list[str]) -> set[str]:
    """Lowercase tokens from ``query_intents`` of length ≥ 3."""
    out: set[str] = set()
    for intent in query_intents or []:
        if not isinstance(intent, str):
            continue
        for t in _TOKEN_SPLIT_RE.split(intent.lower()):
            if len(t) >= _MIN_TOKEN_LEN:
                out.add(t)
    return out


def _bus_hits_score(card: DirectoryCard, canonical_bus: str | None, weight: float) -> float:
    if not canonical_bus or canonical_bus not in card.bus_hits:
        return 0.0
    n = card.bus_hits[canonical_bus]
    capped = min(n, 50)
    # Logarithmic-ish smoothing: 0 → 0, 5 → ~1, 50 → ~2.5 — keeps
    # massive-keyword files from sweeping every slot.
    smoothed = (capped ** 0.5) / 7.0
    return weight * smoothed


def _intent_hits_in(text: str, tokens: set[str]) -> int:
    if not tokens:
        return 0
    text_lower = text.lower()
    return sum(1 for t in tokens if t in text_lower)


def _path_depth(rel_path: str) -> int:
    return rel_path.count("/") + 1 if rel_path else 0


# Scoring


def _score_dir_for_slot(
    card: DirectoryCard,
    slot: SlotGoal,
    weights: dict,
    tokens: set[str] | None = None,
) -> tuple[float, list[str]]:
    """Compute (score, reasons) for one directory against one slot."""
    score = 0.0
    reasons: list[str] = []

    role_bonus_w = float(weights.get("preferred_root_role_bonus", 1.5))
    bus_kw_w = float(weights.get("bus_keyword_weight", 2.0))
    path_intent_w = float(weights.get("path_token_intent_weight", 3.0))

    # 1. role match
    if card.role_hint:
        if slot.preferred_root_roles and card.role_hint in slot.preferred_root_roles:
            bonus = role_bonus_w * 4.0
            score += bonus
            reasons.append(f"role_hint={card.role_hint}+{bonus:.1f}")
        elif slot.negative_root_roles and card.role_hint in slot.negative_root_roles:
            penalty = -role_bonus_w * 3.0
            score += penalty
            reasons.append(f"role_hint={card.role_hint}{penalty:+.1f}")

    # 2. bus keyword density
    bus_score = _bus_hits_score(card, slot.canonical_bus, bus_kw_w)
    if bus_score:
        score += bus_score
        reasons.append(
            f"bus[{slot.canonical_bus}]={card.bus_hits.get(slot.canonical_bus, 0)}+{bus_score:.1f}"
        )

    # 3. intent token match (tokens cached at the per-slot level)
    if tokens is None:
        tokens = _intent_tokens(slot.query_intents)
    if tokens:
        path_hits = _intent_hits_in(card.dir_path, tokens)
        if path_hits:
            bonus = path_intent_w * path_hits
            score += bonus
            reasons.append(f"path_intents={path_hits}+{bonus:.1f}")
        sym_hits = _intent_hits_in(" ".join(card.top_symbols), tokens)
        if sym_hits:
            # Half weight: top_symbols is a sample, not authoritative.
            bonus = (path_intent_w / 2.0) * sym_hits
            score += bonus
            reasons.append(f"symbol_intents={sym_hits}+{bonus:.1f}")

    # 4. depth bonus — only if we already have any positive contribution.
    if score > 0:
        depth = _path_depth(card.dir_path)
        depth_bonus = min(depth * 0.3, 1.5)
        if depth_bonus:
            score += depth_bonus
            reasons.append(f"depth={depth}+{depth_bonus:.1f}")

    # 5. empty-code penalty
    if card.code_file_count == 0 and score > 0:
        score *= 0.3
        reasons.append("no_code_files*0.3")

    return score, reasons


# Public entry


def route_directories_deterministic(
    *,
    bundle: RepoIndexBundle,
    slot_plan: SlotPlan,
    task_spec: TaskSpec,
    max_dirs_per_slot: int | None = None,
    candidate_dirs_per_slot: int | None = None,
    min_score: float = 1.0,
) -> dict[str, DirectoryRoute]:
    """Score every dir_card against every slot in *slot_plan*; return each slot's top ``max_dirs_per_slot`` matches."""
    cfg = load_thresholds()
    weights = cfg.get("scoring", {})
    if max_dirs_per_slot is None:
        max_dirs_per_slot = int(weights.get("max_dirs_per_slot", 8))
    if candidate_dirs_per_slot is None:
        candidate_dirs_per_slot = int(
            weights.get("llm_rerank_candidate_dirs_per_slot", max_dirs_per_slot)
        )
    candidate_dirs_per_slot = max(candidate_dirs_per_slot, max_dirs_per_slot)

    # Cache lowercase card fields used across slot scoring.
    routes: dict[str, DirectoryRoute] = {}

    for slot in slot_plan.slots:
        slot_tokens = _intent_tokens(slot.query_intents)
        scored: list[DirectoryMatch] = []
        for card in bundle.dir_cards:
            score, reasons = _score_dir_for_slot(
                card, slot, weights, tokens=slot_tokens
            )
            if score < min_score:
                continue
            scored.append(DirectoryMatch(card=card, score=score, reasons=reasons))

        scored.sort(
            key=lambda m: (-m.score, m.card.root_id, m.card.dir_path)
        )
        routes[slot.slot_id] = DirectoryRoute(
            slot_id=slot.slot_id,
            matches=scored[:max_dirs_per_slot],
            candidate_matches=scored[:candidate_dirs_per_slot],
        )

    n_covered = sum(1 for r in routes.values() if not r.is_empty)
    logger.info(
        "Deterministic dir router: %d/%d slots got at least one match "
        "(min_score=%.2f, max_per_slot=%d, candidate_per_slot=%d)",
        n_covered,
        len(routes),
        min_score,
        max_dirs_per_slot,
        candidate_dirs_per_slot,
    )

    return routes


__all__ = [
    "DirectoryMatch",
    "DirectoryRoute",
    "route_directories_deterministic",
]
