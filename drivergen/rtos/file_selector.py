"""pipeline step - deterministic file selector."""

from __future__ import annotations

import logging
import re
from dataclasses import dataclass, field

from .config import load_thresholds
from .dir_router import DirectoryRoute
from .types import (
    FileCard,
    RepoIndexBundle,
    SlotGoal,
    SlotPlan,
    TaskSpec,
)

logger = logging.getLogger(__name__)


# Output dataclasses


@dataclass
class FileMatch:
    """One scored file candidate for a slot."""

    card: FileCard
    score: float
    reasons: list[str] = field(default_factory=list)


@dataclass
class FileSelection:
    """One slot's file shortlist (sorted by score, desc)."""

    slot_id: str
    matches: list[FileMatch] = field(default_factory=list)
    candidate_matches: list[FileMatch] = field(default_factory=list)
    fallback_used: bool = False
    """``True`` when the slot's ``DirectoryRoute`` was empty and we made
    no attempt to scan the whole bundle (the selection is therefore
    legitimately empty)."""

    @property
    def top_match(self) -> FileMatch | None:
        return self.matches[0] if self.matches else None

    @property
    def is_empty(self) -> bool:
        return not self.matches


# Tokenization


_TOKEN_SPLIT_RE = re.compile(r"\W+")
_MIN_TOKEN_LEN = 3


def _intent_tokens(query_intents: list[str]) -> set[str]:
    """Extract lowercase tokens of length ≥ 3 from query intents."""
    out: set[str] = set()
    for intent in query_intents or []:
        if not isinstance(intent, str):
            continue
        for t in _TOKEN_SPLIT_RE.split(intent.lower()):
            if len(t) >= _MIN_TOKEN_LEN:
                out.add(t)
    return out


# Helpers


_DEMO_SEGMENTS = ("/demo/", "/demos/", "/example/", "/examples/", "/sample/", "/samples/")
_TEST_SEGMENTS = ("/test/", "/tests/", "/_tests/", "/testing/", "/unittest/")
_DOWNGRADE_KINDS = frozenset({"doc", "build", "config"})

# Expected kinds that indicate a type-only slot; prefer headers for these.
_TYPE_ONLY_EXPECTED_KINDS = frozenset({"macro", "enum", "typedef", "struct", "config"})

# Cap exported-symbol scanning so macro-heavy headers do not dominate
# file-level scoring before per-symbol ranking runs.
_SYMBOL_SCAN_CAP = 60
_SYMBOL_HIT_CAP = 5


def _gather_candidates_from_route(
    bundle: RepoIndexBundle,
    route: DirectoryRoute | None,
) -> list[FileCard]:
    """Collect deduplicated file_cards living under any directory the dir router shortlisted for this slot."""
    if route is None or route.is_empty:
        return []

    seen: set[tuple[str, str]] = set()
    out: list[FileCard] = []
    for m in route.matches:
        for c in bundle.cards_in_dir(m.card.root_id, m.card.dir_path):
            key = (c.root_id, c.path)
            if key in seen:
                continue
            seen.add(key)
            out.append(c)
    return out


def _path_lower_basename(card: FileCard) -> tuple[str, str]:
    p = card.path.lower()
    base = p.rsplit("/", 1)[-1] if "/" in p else p
    return p, base


# Scoring


def _score_file_for_slot(
    card: FileCard,
    slot: SlotGoal,
    weights: dict,
    tokens: set[str] | None = None,
) -> tuple[float, list[str]]:
    """Compute (score, reasons) for one file against one slot."""
    score = 0.0
    reasons: list[str] = []

    role_w = float(weights.get("preferred_root_role_bonus", 1.5))
    bus_w = float(weights.get("bus_keyword_weight", 2.0))
    path_w = float(weights.get("path_token_intent_weight", 3.0))
    demo_pen = float(weights.get("demo_path_penalty", 0.5))
    test_pen = float(weights.get("test_path_penalty", 0.5))

    # 1. role match
    if card.dir_role_hint:
        if slot.preferred_root_roles and card.dir_role_hint in slot.preferred_root_roles:
            bonus = role_w * 4.0
            score += bonus
            reasons.append(f"role={card.dir_role_hint}+{bonus:.1f}")
        elif slot.negative_root_roles and card.dir_role_hint in slot.negative_root_roles:
            penalty = -role_w * 3.0
            score += penalty
            reasons.append(f"role={card.dir_role_hint}{penalty:+.1f}")

    # 2. bus keyword density (log-smoothed)
    if slot.canonical_bus and slot.canonical_bus in card.bus_hits:
        n = card.bus_hits[slot.canonical_bus]
        capped = min(n, 50)
        smoothed = (capped ** 0.5) / 7.0
        bus_score = bus_w * smoothed
        if bus_score:
            score += bus_score
            reasons.append(f"bus[{slot.canonical_bus}]={n}+{bus_score:.1f}")

    # 3+4. path / filename token match — tokens cached per-slot.
    if tokens is None:
        tokens = _intent_tokens(slot.query_intents)
    path_lower, base_lower = _path_lower_basename(card)
    if tokens:
        path_hits = sum(1 for t in tokens if t in path_lower)
        if path_hits:
            bonus = path_w * path_hits
            score += bonus
            reasons.append(f"path={path_hits}+{bonus:.1f}")
        # Count basename hits separately at half weight.
        base_hits = sum(1 for t in tokens if t in base_lower)
        if base_hits:
            bonus = (path_w * 0.5) * base_hits
            score += bonus
            reasons.append(f"fname={base_hits}+{bonus:.1f}")

    # 5. exported_symbols match (capped)
    if tokens and card.exported_symbols:
        sym_hits = 0
        for sym in card.exported_symbols[:_SYMBOL_SCAN_CAP]:
            sym_low = sym.lower()
            if any(t in sym_low for t in tokens):
                sym_hits += 1
                if sym_hits >= _SYMBOL_HIT_CAP:
                    break
        if sym_hits:
            bonus = (path_w * 0.5) * sym_hits
            score += bonus
            reasons.append(f"sym={sym_hits}+{bonus:.1f}")

    # 6a. file_kind degrade (doc / build / config)
    if score > 0 and card.file_kind in _DOWNGRADE_KINDS:
        score *= 0.3
        reasons.append(f"kind={card.file_kind}*0.3")

    # 6a'. type-only slots prefer headers over sources.
    if score > 0 and slot.expected_kinds:
        type_only = (
            len(slot.expected_kinds) > 0
            and all(k in _TYPE_ONLY_EXPECTED_KINDS for k in slot.expected_kinds)
        )
        if type_only:
            if card.file_kind == "header":
                score *= 1.3
                reasons.append("type-only:header*1.3")
            elif card.file_kind == "source":
                score *= 0.5
                reasons.append("type-only:source*0.5")

    # 6b. demo / example / sample path segment penalty
    if score > 0 and any(seg in path_lower for seg in _DEMO_SEGMENTS):
        score *= demo_pen
        reasons.append(f"demo*{demo_pen}")

    # 6c. test path segment penalty
    if score > 0 and any(seg in path_lower for seg in _TEST_SEGMENTS):
        score *= test_pen
        reasons.append(f"test*{test_pen}")

    return score, reasons


# Public entry


def select_files_deterministic(
    *,
    bundle: RepoIndexBundle,
    slot_plan: SlotPlan,
    task_spec: TaskSpec,
    routes: dict[str, DirectoryRoute],
    max_files_per_slot: int | None = None,
    candidate_files_per_slot: int | None = None,
    min_score: float = 1.0,
) -> dict[str, FileSelection]:
    """Pick the top ``max_files_per_slot`` files for every slot."""
    cfg = load_thresholds()
    weights = cfg.get("scoring", {})
    file_cfg = cfg.get("candidate_ranker", {})
    if max_files_per_slot is None:
        max_files_per_slot = int(file_cfg.get("max_candidate_files_per_slot", 12))
    if candidate_files_per_slot is None:
        candidate_files_per_slot = int(
            file_cfg.get("llm_rerank_candidate_files_per_slot", max_files_per_slot)
        )
    candidate_files_per_slot = max(candidate_files_per_slot, max_files_per_slot)

    selections: dict[str, FileSelection] = {}

    for slot in slot_plan.slots:
        route = routes.get(slot.slot_id)
        candidates = _gather_candidates_from_route(bundle, route)
        if not candidates:
            # Leave empty routes empty so gap diagnosis can handle them.
            selections[slot.slot_id] = FileSelection(
                slot_id=slot.slot_id,
                matches=[],
                candidate_matches=[],
                fallback_used=False,
            )
            continue

        slot_tokens = _intent_tokens(slot.query_intents)
        scored: list[FileMatch] = []
        for card in candidates:
            score, reasons = _score_file_for_slot(
                card, slot, weights, tokens=slot_tokens
            )
            if score < min_score:
                continue
            scored.append(FileMatch(card=card, score=score, reasons=reasons))

        scored.sort(
            key=lambda m: (-m.score, m.card.root_id, m.card.path)
        )
        selections[slot.slot_id] = FileSelection(
            slot_id=slot.slot_id,
            matches=scored[:max_files_per_slot],
            candidate_matches=scored[:candidate_files_per_slot],
            fallback_used=False,
        )

    n_covered = sum(1 for s in selections.values() if not s.is_empty)
    logger.info(
        "Deterministic file selector: %d/%d slots got at least one file "
        "(min_score=%.2f, max_per_slot=%d, candidate_per_slot=%d)",
        n_covered,
        len(selections),
        min_score,
        max_files_per_slot,
        candidate_files_per_slot,
    )

    return selections


__all__ = [
    "FileMatch",
    "FileSelection",
    "select_files_deterministic",
]
