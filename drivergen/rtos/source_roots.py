"""Resolve a task package's RTOS bundle into a flat list of :class:`drivergen.rtos.types.SourceRoot` objects."""

from __future__ import annotations

import hashlib
import json
import logging
import re
import subprocess
from dataclasses import replace
from pathlib import Path

from .aliases import canonicalize_rtos_id
from .types import SourceRoot

logger = logging.getLogger(__name__)


# Path resolution


# Local fallbacks for direct module loading.
_PACKAGE_ROOT = Path(__file__).resolve().parent.parent  # .../drivergen/
_PROJECT_ROOT = _PACKAGE_ROOT.parent
_DATA_ROOT = _PROJECT_ROOT / "data"
_DEFAULT_MANIFEST_PATH = _DATA_ROOT / "rtos" / "manifest.json"


def _resolve_repo_path(path_text: str | Path) -> Path:
    """Translate a manifest-relative path string into a real filesystem path."""
    path = Path(path_text)
    if path.is_absolute():
        return path

    candidates: list[Path] = []
    if path.parts and path.parts[0].lower() == _PROJECT_ROOT.name.lower():
        trimmed = Path(*path.parts[1:])
        candidates.append((_PROJECT_ROOT / trimmed).resolve())
        candidates.append((_PROJECT_ROOT.parent / trimmed).resolve())
    candidates.append((_PROJECT_ROOT / path).resolve())
    candidates.append((_PROJECT_ROOT.parent / path).resolve())

    seen: set[Path] = set()
    for candidate in candidates:
        if candidate in seen:
            continue
        seen.add(candidate)
        if candidate.exists():
            return candidate
    return candidates[0]


# Manifest loading


def _load_manifest(manifest_path: Path | None = None) -> dict:
    path = manifest_path or _DEFAULT_MANIFEST_PATH
    if not path.exists():
        raise FileNotFoundError(f"manifest.json not found at {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def _find_entry(manifest: dict, bundle_id: str) -> dict:
    """Locate a manifest entry by exact (lowercased) id."""
    bundle_id_norm = (bundle_id or "").strip().lower()
    if not bundle_id_norm:
        raise ValueError("bundle_id must be a non-empty string")
    repos = manifest.get("repositories", [])
    for entry in repos:
        if (entry.get("id") or "").strip().lower() == bundle_id_norm:
            return entry
    known = ", ".join(sorted(e.get("id", "") for e in repos))
    raise KeyError(f"bundle_id '{bundle_id}' not found in manifest. Known ids: {known}")


# Role assignment


def _normalize_text(value: str | None) -> str:
    return (value or "").replace("\\", "/").lower()


def _contains_word(text: str, keyword: str) -> bool:
    if not keyword:
        return False
    return re.search(
        rf"(?:^|[^A-Za-z0-9_]){re.escape(keyword.lower())}(?:$|[^A-Za-z0-9_])",
        text.lower(),
    ) is not None


# Ordered component-kind rules; first match wins.
_KIND_ROLE_TABLE: tuple[tuple[str, frozenset[str]], ...] = (
    ("rtos_core",         frozenset({"kernel", "runtime"})),
    ("driver_framework",  frozenset({"driver_framework"})),
    ("document",          frozenset({"docs"})),
    # Keep the broader document rule before the shorter doc rule.
    ("doc",               frozenset({"docs"})),
    ("vendor_sdk",        frozenset({"vendor_hal", "driver_framework", "board_integration"})),
    ("vendor_bsp",        frozenset({"vendor_hal", "driver_framework", "board_integration"})),
    ("middleware",        frozenset({"driver_framework", "vendor_hal"})),
    ("chipadaptation",    frozenset({"driver_framework", "board_integration"})),
    ("hal",               frozenset({"driver_framework", "board_integration"})),
    ("platform",          frozenset({"driver_framework", "board_integration"})),
    ("integration",       frozenset({"driver_framework", "board_integration"})),
    ("board",             frozenset({"driver_framework", "board_integration"})),
)

_DEFAULT_COMPONENT_ROLES: frozenset[str] = frozenset({"board_integration"})


def _roles_for_component_kind(kind: str | None) -> frozenset[str]:
    norm = _normalize_text(kind)
    if not norm:
        return _DEFAULT_COMPONENT_ROLES
    for token, roles in _KIND_ROLE_TABLE:
        if token in norm:
            return roles
    return _DEFAULT_COMPONENT_ROLES


# Derive companion roles from manifest text.
_COMPANION_DOC_HINTS = ("document", "docs")
_COMPANION_INTEGRATION_WORDS = (
    "hal",
    "board",
    "platform",
    "integration",
    "middleware",
)
_COMPANION_INTEGRATION_SUBSTRINGS = ("chipadaptation", "vendor_bsp")


def _roles_for_companion(companion: dict) -> frozenset[str]:
    joined = " ".join(
        [
            companion.get("id", "") or "",
            companion.get("display_name", "") or "",
            companion.get("note", "") or "",
            companion.get("path", "") or "",
        ]
    )
    norm = _normalize_text(joined)
    if any(_contains_word(norm, w) for w in _COMPANION_DOC_HINTS) or "/docs" in norm:
        return frozenset({"docs"})
    if any(_contains_word(norm, w) for w in _COMPANION_INTEGRATION_WORDS) or any(
        s in norm for s in _COMPANION_INTEGRATION_SUBSTRINGS
    ):
        return frozenset({"driver_framework", "board_integration"})
    return _DEFAULT_COMPONENT_ROLES


# SHA and fingerprint


# Git SHA cache keyed by absolute repository path.
_ROOT_SHA_CACHE: dict[str, str] = {}


def reset_root_sha_cache() -> None:
    """Clear the module-level root-SHA cache.  Useful in tests that
    swap in temporary repository paths.
    """
    _ROOT_SHA_CACHE.clear()


def _compute_root_sha(repo_root: Path) -> str:
    """Cheap repo identifier for cache invalidation."""
    if not repo_root.exists():
        return ""

    cache_key = str(repo_root.resolve())
    cached = _ROOT_SHA_CACHE.get(cache_key)
    if cached is not None:
        return cached

    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True,
            text=True,
            cwd=repo_root,
            # Avoid waiting on slow or stuck git metadata reads.
            timeout=5,
        )
        if result.returncode == 0:
            sha = result.stdout.strip()
            if sha:
                _ROOT_SHA_CACHE[cache_key] = sha
                return sha
    except (FileNotFoundError, subprocess.TimeoutExpired) as exc:
        logger.debug("git sha lookup failed for %s: %s", repo_root, exc)

    h = hashlib.md5()
    try:
        headers = sorted(repo_root.rglob("*.h"))[:200]
    except OSError:
        return ""
    for p in headers:
        try:
            rel = p.relative_to(repo_root).as_posix()
        except ValueError:
            rel = p.name
        try:
            size = p.stat().st_size
        except OSError:
            size = 0
        h.update(f"{rel}:{size}\n".encode("utf-8"))
    sha = h.hexdigest()[:12]
    _ROOT_SHA_CACHE[cache_key] = sha
    return sha


# Public API


def _canonical_rtos_id(bundle_id: str, entry: dict) -> str:
    """Return the canonical RTOS id (matches ``data/rtos/scope_map/<id>.json``)."""
    target = (entry.get("rtos_target") or "").strip().lower()
    if target:
        return canonicalize_rtos_id(target)
    return canonicalize_rtos_id(bundle_id)


def _component_root_id(bundle_id: str, component: dict, idx: int) -> str:
    component_id = (component.get("id") or "").strip()
    if component_id:
        return f"{bundle_id}:{component_id}:{idx}"
    kind = (component.get("kind") or "unknown_kind").strip().lower().replace(" ", "_")
    return f"{bundle_id}:{kind}:{idx}"


def _companion_root_id(bundle_id: str, companion: dict, idx: int) -> str:
    companion_id = (companion.get("id") or "").strip()
    if companion_id:
        return f"{bundle_id}:companion:{companion_id}"
    return f"{bundle_id}:companion:{idx}"


def resolve_source_roots(
    bundle_id: str,
    *,
    manifest_path: Path | None = None,
    compute_sha: bool = True,
) -> list[SourceRoot]:
    """Expand a manifest bundle into the flat :class:`SourceRoot` list consumed by the rest of."""
    manifest = _load_manifest(manifest_path)
    entry = _find_entry(manifest, bundle_id)
    rtos_scope = _canonical_rtos_id(bundle_id, entry)

    roots: list[SourceRoot] = []

    components: list[dict] = list(entry.get("components") or [])
    companions: list[dict] = []
    if entry.get("companion"):
        companions.append(entry["companion"])
    companions.extend(entry.get("companions") or [])

    if components:
        # Multi-component bundle: every component is its own root with
        # roles derived from kind.
        for idx, component in enumerate(components):
            comp_path = component.get("path")
            if not comp_path:
                logger.warning(
                    "Component '%s' under bundle '%s' has no path; skipping",
                    component.get("id") or component.get("kind"),
                    bundle_id,
                )
                continue
            resolved = _resolve_repo_path(comp_path)
            roots.append(
                SourceRoot(
                    root_id=_component_root_id(bundle_id, component, idx),
                    path=resolved,
                    roles=_roles_for_component_kind(component.get("kind")),
                    priority=1.0,
                    rtos_scope_id=rtos_scope,
                )
            )
    else:
        # Single roots cover all roles unless companions split them.
        main_path = entry.get("path")
        if not main_path:
            raise ValueError(
                f"Manifest entry '{bundle_id}' has neither 'components' nor 'path'."
            )
        # Shrink main-root roles only for roles covered by companions.
        _companion_roles: set[str] = set()
        for c in companions:
            _companion_roles.update(_roles_for_companion(c))
        if _companion_roles:
            main_roles = frozenset(
                {"kernel", "runtime", "driver_framework", "vendor_hal", "board_integration"}
                - _companion_roles
            )
        else:
            main_roles = frozenset(
                {"kernel", "runtime", "driver_framework", "board_integration"}
            )
        roots.append(
            SourceRoot(
                root_id=f"{bundle_id}:single:0",
                path=_resolve_repo_path(main_path),
                roles=main_roles,
                priority=1.0,
                rtos_scope_id=rtos_scope,
            )
        )

    for idx, companion in enumerate(companions):
        comp_path = companion.get("path")
        if not comp_path:
            logger.warning(
                "Companion under bundle '%s' has no path; skipping", bundle_id
            )
            continue
        roots.append(
            SourceRoot(
                root_id=_companion_root_id(bundle_id, companion, idx),
                path=_resolve_repo_path(comp_path),
                roles=_roles_for_companion(companion),
                priority=1.0,
                rtos_scope_id=rtos_scope,
            )
        )

    if compute_sha:
        roots = [
            replace(r, sha=_compute_root_sha(r.path))
            if not r.sha
            else r
            for r in roots
        ]

    if not roots:
        raise ValueError(
            f"Manifest entry '{bundle_id}' produced 0 source roots — "
            "every component/companion was missing a path?"
        )
    return roots


def root_set_hash(roots: list[SourceRoot]) -> str:
    """Stable digest used as the cache key prefix for RepoIndexBundle."""
    h = hashlib.sha1()
    for r in sorted(roots, key=lambda x: x.root_id):
        roles_token = ",".join(sorted(r.roles))
        h.update(f"{r.root_id}|{r.sha}|{roles_token}\n".encode("utf-8"))
    return h.hexdigest()[:16]


__all__ = [
    "resolve_source_roots",
    "root_set_hash",
]
