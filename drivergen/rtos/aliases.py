"""Canonical RTOS id helpers derived from the RTOS manifest."""

from __future__ import annotations

import json
import logging
from functools import lru_cache
from pathlib import Path

logger = logging.getLogger(__name__)


def _manifest_path() -> Path:
    # Mirror the path resolution in source_roots.py so this module can
    # be imported standalone in unit tests.
    package_root = Path(__file__).resolve().parent.parent
    project_root = package_root.parent
    return project_root / "data" / "rtos" / "manifest.json"


@lru_cache(maxsize=1)
def _load_manifest_aliases() -> dict[str, str]:
    path = _manifest_path()
    if not path.exists():
        return {}
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        logger.warning("alias loader: cannot parse %s: %s", path, exc)
        return {}
    out: dict[str, str] = {}
    repos = manifest.get("repositories") or []
    for entry in repos:
        if not isinstance(entry, dict):
            continue
        bundle_id = (entry.get("id") or "").strip().lower()
        if not bundle_id:
            continue
        target = (entry.get("rtos_target") or "").strip().lower()
        out[bundle_id] = target or bundle_id
    return out


def reset_alias_cache() -> None:
    """Clear the manifest-derived alias cache.  Useful in tests that
    swap in a temporary manifest path between runs.
    """
    _load_manifest_aliases.cache_clear()


def canonicalize_rtos_id(value: str | None) -> str:
    """Lower-case and apply manifest-derived aliases."""
    norm = (value or "").strip().lower()
    if not norm:
        return ""
    manifest_aliases = _load_manifest_aliases()
    if norm in manifest_aliases:
        return manifest_aliases[norm]
    return norm


__all__ = ["canonicalize_rtos_id", "reset_alias_cache"]
