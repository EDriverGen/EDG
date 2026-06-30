"""pipeline step - 1-hop include / counterpart expansion."""

from __future__ import annotations

import logging
from pathlib import Path

from .deep_parser import ParsedFileBundle, deep_parse_files
from .types import FileCard, RepoIndexBundle

logger = logging.getLogger(__name__)

# Cap extra files per root so dense include graphs cannot explode parse time.
DEFAULT_MAX_EXTRAS_PER_ROOT = 64

# C-family suffixes used by counterpart discovery.
_CFAMILY_HEADER = (".h", ".hpp", ".hh", ".hxx")
_CFAMILY_SOURCE = (".c", ".cpp", ".cc", ".cxx")


# Internal helpers


def _split_dir_and_name(rel_path: str) -> tuple[str, str]:
    """Split ``a/b/c.h`` into ``("a/b", "c.h")`` and ``"foo.h"`` into ``("", "foo.h")``."""
    if "/" not in rel_path:
        return "", rel_path
    head, _, tail = rel_path.rpartition("/")
    return head, tail


def _counterpart_path(rel_path: str) -> str | None:
    """``foo.h`` → ``foo.c`` (and vice versa); ``None`` for non-C-family."""
    for h in _CFAMILY_HEADER:
        if rel_path.endswith(h):
            return rel_path[: -len(h)] + ".c"
    for s in _CFAMILY_SOURCE:
        if rel_path.endswith(s):
            return rel_path[: -len(s)] + ".h"
    return None


def _is_likely_system_include(inc: str) -> bool:
    """Cheap structural filter - drop includes that obviously can't resolve to a file inside any of our manifest roots."""
    if not inc or inc.startswith(("/", ".")):
        return True
    if "." not in inc.rsplit("/", 1)[-1]:
        return True
    return False


def _resolve_same_root_include(
    bundle: RepoIndexBundle,
    root_id: str,
    cur_dir: str,
    inc: str,
) -> FileCard | None:
    """Find a same-root FileCard whose path ends with ``inc`` (or ``"/" + inc``)."""
    cards_in_root = bundle.cards_in_root(root_id)
    candidates: list[FileCard] = []
    for c in cards_in_root:
        if c.path == inc or c.path.endswith("/" + inc):
            candidates.append(c)

    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]

    # Prefer a candidate from the current directory, then any common
    # parent prefix, then the alphabetically-first remaining one.
    cur_dir_norm = cur_dir.rstrip("/")

    def _prefix_score(card: FileCard) -> tuple[int, str]:
        # 0 = same dir, 1 = ancestor dir, 2 = anywhere else.
        cand_dir, _ = _split_dir_and_name(card.path)
        if cand_dir == cur_dir_norm:
            return (0, card.path)
        if cur_dir_norm and (cand_dir + "/").startswith(cur_dir_norm + "/"):
            return (1, card.path)
        return (2, card.path)

    candidates.sort(key=_prefix_score)
    return candidates[0]


def _find_card_in_root(
    bundle: RepoIndexBundle, root_id: str, target_path: str
) -> FileCard | None:
    for c in bundle.cards_in_root(root_id):
        if c.path == target_path:
            return c
    return None


# Public entry


def expand_includes_and_counterparts(
    *,
    bundle: RepoIndexBundle,
    parsed_bundles: dict[str, ParsedFileBundle],
    max_extras_per_root: int | None = None,
) -> dict[str, ParsedFileBundle]:
    """Augment ``parsed_bundles`` with same-root includes + counterparts."""
    if max_extras_per_root is None:
        from .config import load_thresholds  # local to avoid cycle
        cfg = load_thresholds().get("indexer", {})
        max_extras_per_root = int(
            cfg.get("max_extras_per_root", DEFAULT_MAX_EXTRAS_PER_ROOT)
        )
    # Collect one-hop include expansion seeds from the current snapshot.
    starting_keys = set(parsed_bundles.keys())
    extras: dict[tuple[str, str], list[str]] = {}

    for b in list(parsed_bundles.values()):
        root_id = b.card.root_id
        cur_dir, _ = _split_dir_and_name(b.card.path)

        # 1a. Same-root includes.
        for inc in b.parsed.include_graph or []:
            if _is_likely_system_include(inc):
                continue
            target = _resolve_same_root_include(bundle, root_id, cur_dir, inc)
            if target is None:
                continue
            key = (root_id, target.path)
            new_file_key = f"{root_id}::{target.path}"
            if new_file_key in starting_keys:
                continue
            extras.setdefault(key, []).extend(b.slot_ids)

        # 1b. .h ↔ .c counterpart.
        ctp_path = _counterpart_path(b.card.path)
        if ctp_path is None:
            continue
        ctp_card = _find_card_in_root(bundle, root_id, ctp_path)
        if ctp_card is None:
            continue
        new_file_key = f"{root_id}::{ctp_card.path}"
        if new_file_key in starting_keys:
            continue
        extras.setdefault((root_id, ctp_card.path), []).extend(b.slot_ids)

    if not extras:
        logger.info("Include/counterpart expansion: no new files to add")
        return parsed_bundles

    # 2. Per-root cap.  Iterate keys in stable order so cache hashes
    #    don't depend on dict ordering.
    capped: dict[tuple[str, str], list[str]] = {}
    counts_per_root: dict[str, int] = {}
    skipped_over_cap: dict[str, int] = {}
    for key in sorted(extras.keys()):
        root_id = key[0]
        if counts_per_root.get(root_id, 0) >= max_extras_per_root:
            skipped_over_cap[root_id] = skipped_over_cap.get(root_id, 0) + 1
            continue
        capped[key] = extras[key]
        counts_per_root[root_id] = counts_per_root.get(root_id, 0) + 1

    if skipped_over_cap:
        logger.info(
            "Include/counterpart expansion: per-root cap %d reached for %s "
            "(extras skipped above cap)",
            max_extras_per_root,
            ", ".join(f"{r}+{n}" for r, n in skipped_over_cap.items()),
        )

    # 3. Resolve to FileCard + abs paths.
    root_id_to_path = {r.root_id: r.path for r in bundle.roots}
    cards_lookup: dict[tuple[str, str], FileCard] = {}
    for card in bundle.file_cards:
        key = (card.root_id, card.path)
        if key in capped:
            cards_lookup[key] = card

    paths_to_parse: list[tuple[Path, str]] = []
    keys_in_order: list[tuple[str, str]] = []
    skipped_missing = 0
    for key in capped.keys():
        card = cards_lookup.get(key)
        if card is None:
            # Should not happen — cards_in_root just gave it to us.
            skipped_missing += 1
            continue
        if card.abs_path:
            abs_path = Path(card.abs_path)
        else:
            root_path = root_id_to_path.get(card.root_id)
            if root_path is None:
                skipped_missing += 1
                continue
            abs_path = root_path / card.path
        if not abs_path.exists():
            skipped_missing += 1
            continue
        paths_to_parse.append((abs_path, card.path))
        keys_in_order.append(key)

    # 4. Parse new files (single tree-sitter pass).
    if not paths_to_parse:
        logger.info("Include/counterpart expansion: 0 new files (skipped %d missing)", skipped_missing)
        return parsed_bundles

    parsed_list = deep_parse_files(paths_to_parse)

    # 5. Merge into the parsed_bundles dict, with provenance.
    n_added = 0
    for key, parsed in zip(keys_in_order, parsed_list):
        card = cards_lookup[key]
        slot_ids = sorted(set(capped[key]))
        new_b = ParsedFileBundle(parsed=parsed, card=card, slot_ids=slot_ids)
        parsed_bundles[new_b.file_key] = new_b
        n_added += 1

    logger.info(
        "Include/counterpart expansion: added %d files (cap=%d/root, skipped %d missing)",
        n_added,
        max_extras_per_root,
        skipped_missing,
    )
    return parsed_bundles


__all__ = [
    "expand_includes_and_counterparts",
    "DEFAULT_MAX_EXTRAS_PER_ROOT",
]
