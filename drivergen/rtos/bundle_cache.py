"""pipeline step - RepoIndexBundle on-disk cache."""

from __future__ import annotations

import dataclasses
import json
import logging
import os
import tempfile
from pathlib import Path

from .repo_index import build_repo_index_bundle, scope_map_hash_for
from .scope_map import ScopeMapEntry
from .source_roots import root_set_hash
from .types import (
    DirectoryCard,
    FileCard,
    IncludeEdge,
    RepoIndexBundle,
    SourceRoot,
    SymbolSketch,
)

logger = logging.getLogger(__name__)


# Cache root resolution


_PACKAGE_ROOT = Path(__file__).resolve().parent.parent  # .../drivergen/
_PROJECT_ROOT = _PACKAGE_ROOT.parent
_DEFAULT_CACHE_ROOT = _PROJECT_ROOT / "data" / "cache" / "rtos_index_bundle"


def cache_dir_for(
    *,
    source_roots: list[SourceRoot],
    scope_map: ScopeMapEntry | None,
    indexer_version: str,
    cache_root: Path | None = None,
) -> Path:
    """Compute the on-disk cache directory for a given ``(roots, scope_map, indexer_version)`` triple."""
    rs_hash = root_set_hash(source_roots)
    sm_hash = scope_map_hash_for(scope_map)
    base = cache_root or _DEFAULT_CACHE_ROOT
    return base / indexer_version / f"{rs_hash}_{sm_hash}"


# Serialization


def _to_jsonable(obj):
    """Recursively convert dataclasses / Path / frozenset to JSON types."""
    if obj is None or isinstance(obj, (str, int, float, bool)):
        return obj
    if isinstance(obj, Path):
        return str(obj)
    if isinstance(obj, frozenset):
        # Sort string-only frozensets for stable output; otherwise keep
        # element order via list.
        if obj and all(isinstance(e, str) for e in obj):
            return sorted(obj)
        return list(obj)
    if isinstance(obj, set):
        return sorted(obj) if all(isinstance(e, str) for e in obj) else list(obj)
    if dataclasses.is_dataclass(obj):
        return {f.name: _to_jsonable(getattr(obj, f.name)) for f in dataclasses.fields(obj)}
    if isinstance(obj, (list, tuple)):
        return [_to_jsonable(x) for x in obj]
    if isinstance(obj, dict):
        return {str(k): _to_jsonable(v) for k, v in obj.items()}
    raise TypeError(f"Cannot serialise object of type {type(obj).__name__}")


def _bundle_to_dict(bundle: RepoIndexBundle) -> dict:
    return _to_jsonable(bundle)


# Deserialization


def _source_root_from_dict(d: dict) -> SourceRoot:
    return SourceRoot(
        root_id=d["root_id"],
        path=Path(d["path"]),
        roles=frozenset(d.get("roles") or []),
        priority=float(d.get("priority", 1.0)),
        rtos_scope_id=d.get("rtos_scope_id", ""),
        sha=d.get("sha", ""),
    )


def _symbol_sketch_from_dict(d: dict) -> SymbolSketch:
    return SymbolSketch(
        name=d["name"],
        kind=d.get("kind", "function"),
        signature=d.get("signature"),
        file=d.get("file", ""),
        root_id=d.get("root_id", ""),
        root_roles=frozenset(d.get("root_roles") or []),
        path_tokens=list(d.get("path_tokens") or []),
        file_kind=d.get("file_kind"),
        dir_role_hint=d.get("dir_role_hint"),
        include_context=list(d.get("include_context") or []),
        nearby_text=d.get("nearby_text"),
    )


def _file_card_from_dict(d: dict) -> FileCard:
    return FileCard(
        root_id=d["root_id"],
        path=d["path"],
        abs_path=d.get("abs_path", ""),
        file_kind=d.get("file_kind", "config"),
        dir_role_hint=d.get("dir_role_hint"),
        exported_symbols=list(d.get("exported_symbols") or []),
        candidate_symbols=[
            _symbol_sketch_from_dict(s) for s in d.get("candidate_symbols") or []
        ],
        bus_hits=dict(d.get("bus_hits") or {}),
        runtime_hits=dict(d.get("runtime_hits") or {}),
        board_hits=dict(d.get("board_hits") or {}),
        mcu_affinity_score=float(d.get("mcu_affinity_score") or 0.0),
        ecosystem_score=float(d.get("ecosystem_score") or 1.0),
        include_deps=list(d.get("include_deps") or []),
        short_summary=d.get("short_summary", ""),
    )


def _dir_card_from_dict(d: dict) -> DirectoryCard:
    return DirectoryCard(
        root_id=d["root_id"],
        dir_path=d["dir_path"],
        file_count=int(d.get("file_count") or 0),
        code_file_count=int(d.get("code_file_count") or 0),
        role_hint=d.get("role_hint"),
        bus_hits=dict(d.get("bus_hits") or {}),
        runtime_hits=dict(d.get("runtime_hits") or {}),
        board_hits=dict(d.get("board_hits") or {}),
        top_symbols=list(d.get("top_symbols") or []),
        children=list(d.get("children") or []),
    )


def _include_edge_from_dict(d: dict) -> IncludeEdge:
    return IncludeEdge(
        src_root_id=d["src_root_id"],
        src_path=d["src_path"],
        dst_path=d["dst_path"],
    )


def _bundle_from_dict(d: dict) -> RepoIndexBundle:
    return RepoIndexBundle(
        roots=[_source_root_from_dict(r) for r in d.get("roots") or []],
        file_cards=[_file_card_from_dict(c) for c in d.get("file_cards") or []],
        dir_cards=[_dir_card_from_dict(c) for c in d.get("dir_cards") or []],
        symbol_sketches=[
            _symbol_sketch_from_dict(s) for s in d.get("symbol_sketches") or []
        ],
        include_edges=[
            _include_edge_from_dict(e) for e in d.get("include_edges") or []
        ],
        root_shas=dict(d.get("root_shas") or {}),
        scope_map_hash=d.get("scope_map_hash", ""),
        indexer_version=d.get("indexer_version", ""),
    )


# I/O


def _atomic_write_text(path: Path, content: str) -> None:
    """Write *content* to *path* via a temp file + os.replace."""
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(
        prefix=path.name + ".",
        suffix=".tmp",
        dir=str(path.parent),
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(content)
        os.replace(tmp_name, path)
    except Exception:
        # On failure remove the temp file (best-effort)
        try:
            os.remove(tmp_name)
        except OSError:
            pass
        raise


def _build_cache_descriptor(
    *,
    source_roots: list[SourceRoot],
    scope_map: ScopeMapEntry | None,
    indexer_version: str,
) -> dict:
    return {
        "indexer_version": indexer_version,
        "root_set_hash": root_set_hash(source_roots),
        "scope_map_hash": scope_map_hash_for(scope_map),
        "scope_map_version": scope_map.scope_map_version if scope_map else None,
        "scope_map_rtos_id": scope_map.rtos_id if scope_map else None,
        "roots": [
            {
                "root_id": r.root_id,
                "path": str(r.path),
                "roles": sorted(r.roles),
                "sha": r.sha,
                "priority": r.priority,
                "rtos_scope_id": r.rtos_scope_id,
            }
            for r in source_roots
        ],
    }


def save_repo_index_bundle(
    bundle: RepoIndexBundle,
    cache_dir: Path,
    *,
    descriptor: dict | None = None,
) -> Path:
    """Atomically write *bundle* to ``cache_dir/repo_index_bundle.json``."""
    cache_dir.mkdir(parents=True, exist_ok=True)
    bundle_path = cache_dir / "repo_index_bundle.json"
    key_path = cache_dir / "cache_key.json"

    payload = json.dumps(_bundle_to_dict(bundle), ensure_ascii=False, indent=2)
    _atomic_write_text(bundle_path, payload)

    if descriptor is None:
        descriptor = {
            "indexer_version": bundle.indexer_version,
            "scope_map_hash": bundle.scope_map_hash,
            "root_set_hash": root_set_hash(bundle.roots),
            "roots": [
                {
                    "root_id": r.root_id,
                    "path": str(r.path),
                    "roles": sorted(r.roles),
                    "sha": r.sha,
                }
                for r in bundle.roots
            ],
        }
    _atomic_write_text(
        key_path, json.dumps(descriptor, ensure_ascii=False, indent=2)
    )

    logger.info(
        "Saved repo index bundle to %s (%d KB, %d file_cards)",
        bundle_path,
        bundle_path.stat().st_size // 1024,
        len(bundle.file_cards),
    )
    return bundle_path


def load_repo_index_bundle(
    cache_dir: Path,
    *,
    expected_indexer_version: str | None = None,
    expected_scope_map_hash: str | None = None,
    expected_root_set_hash: str | None = None,
) -> RepoIndexBundle | None:
    """Load a cached bundle."""
    bundle_path = cache_dir / "repo_index_bundle.json"
    if not bundle_path.exists():
        return None
    try:
        data = json.loads(bundle_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        logger.warning("Cached bundle at %s is corrupt: %s", bundle_path, exc)
        return None

    bundle = _bundle_from_dict(data)

    if (
        expected_indexer_version is not None
        and bundle.indexer_version != expected_indexer_version
    ):
        logger.info(
            "Cache miss: indexer_version mismatch %s vs %s",
            bundle.indexer_version,
            expected_indexer_version,
        )
        return None
    if (
        expected_scope_map_hash is not None
        and bundle.scope_map_hash != expected_scope_map_hash
    ):
        logger.info(
            "Cache miss: scope_map_hash mismatch %s vs %s",
            bundle.scope_map_hash,
            expected_scope_map_hash,
        )
        return None
    if (
        expected_root_set_hash is not None
        and root_set_hash(bundle.roots) != expected_root_set_hash
    ):
        logger.info(
            "Cache miss: root_set_hash mismatch (cache may have outlived "
            "an underlying root sha change)",
        )
        return None

    logger.info(
        "Loaded cached repo index bundle from %s (%d file_cards, scope_map_hash=%s)",
        bundle_path,
        len(bundle.file_cards),
        bundle.scope_map_hash,
    )
    return bundle


# High-level wrapper


def build_or_load_repo_index_bundle(
    *,
    source_roots: list[SourceRoot],
    scope_map: ScopeMapEntry | None = None,
    indexer_version: str | None = None,
    cache_root: Path | None = None,
    use_cache: bool = True,
) -> RepoIndexBundle:
    """Try cache first; on miss, build + save."""
    if indexer_version is None:
        # Late import to avoid pulling config too eagerly at module load
        from .config import load_thresholds

        indexer_version = str(
            load_thresholds().get("cache", {}).get("indexer_version", "1.0")
        )

    cache_dir = cache_dir_for(
        source_roots=source_roots,
        scope_map=scope_map,
        indexer_version=indexer_version,
        cache_root=cache_root,
    )

    if use_cache:
        cached = load_repo_index_bundle(
            cache_dir,
            expected_indexer_version=indexer_version,
            expected_scope_map_hash=scope_map_hash_for(scope_map),
            expected_root_set_hash=root_set_hash(source_roots),
        )
        if cached is not None:
            return cached

    bundle = build_repo_index_bundle(
        source_roots=source_roots,
        scope_map=scope_map,
        indexer_version=indexer_version,
    )

    if use_cache:
        descriptor = _build_cache_descriptor(
            source_roots=source_roots,
            scope_map=scope_map,
            indexer_version=indexer_version,
        )
        try:
            save_repo_index_bundle(bundle, cache_dir, descriptor=descriptor)
        except OSError as exc:
            logger.warning(
                "Failed to save repo index bundle to %s: %s", cache_dir, exc
            )

    return bundle


__all__ = [
    "cache_dir_for",
    "save_repo_index_bundle",
    "load_repo_index_bundle",
    "build_or_load_repo_index_bundle",
]
