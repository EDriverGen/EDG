"""Public entry point for task-specific :class:`ScopeMapEntry` synthesis."""

from __future__ import annotations

import hashlib
import json
import logging
import os
import time
from pathlib import Path
from typing import Iterable

from ..config import SCOPE_MAP_DIR
from ..llm_infra import make_budget_tracker
from ..scope_map import (
    ScopeMapEntry,
    _build_entry,
)
from ..types import SourceRoot
from .triage import (
    PROMPT_VERSION,
    ROLES,
    canonical_mcu_family,
    derive_scope_fragment,
    role_for_bus,
    run_triage_for_role,
)

logger = logging.getLogger(__name__)


SCOPE_LLM_CACHE_DIR = SCOPE_MAP_DIR / "_llm_cache"


# Mode


_MODES = {"auto", "force", "cache_only", "off"}


def _resolve_mode(explicit: str | None) -> str:
    mode = (explicit or os.environ.get("DRIVERGEN_SCOPE_LLM_MODE") or "auto").lower()
    if mode not in _MODES:
        logger.warning(
            "DRIVERGEN_SCOPE_LLM_MODE=%r is not recognised; defaulting to 'auto'",
            mode,
        )
        return "auto"
    return mode


# Cache key


def _normalise_bus_kinds(bus_kinds: Iterable[str] | None) -> tuple[str, ...]:
    if not bus_kinds:
        return ()
    # Canonicalise bus-kind aliases so gpio_timing / gpio_pulse / etc.
    # share the same scope-map cache as plain gpio.
    _BUS_ALIASES: dict[str, str] = {
        "gpio_timing": "gpio",
        "gpio_pulse": "gpio",
        "gpio_oneshot": "gpio",
        "gpio_pulse_width": "gpio",
    }
    seen: set[str] = set()
    out: list[str] = []
    for b in bus_kinds:
        if not b:
            continue
        bb = b.strip().lower()
        bb = _BUS_ALIASES.get(bb, bb)
        if bb and bb not in seen:
            seen.add(bb)
            out.append(bb)
    return tuple(sorted(out))


def _normalise_mcu_for_key(mcu_family: str | None) -> str:
    """Cache-key form of an MCU family."""
    if not mcu_family:
        return "mcuany"
    canonical = canonical_mcu_family(mcu_family)
    if canonical:
        return canonical
    return mcu_family.strip().replace(" ", "").replace("/", "_")


def cache_key_for(
    *,
    rtos_id: str,
    mcu_family: str | None,
    bus_kinds: tuple[str, ...] = (),
    prompt_version: str = PROMPT_VERSION,
) -> str:
    bus_kinds = _normalise_bus_kinds(bus_kinds)
    if bus_kinds:
        bus_hash = hashlib.sha1(
            "|".join(bus_kinds).encode("utf-8")
        ).hexdigest()[:8]
    else:
        bus_hash = "nobus"
    return (
        f"{rtos_id}__{_normalise_mcu_for_key(mcu_family)}__{bus_hash}__{prompt_version}"
    )


def cache_path_for(
    *,
    rtos_id: str,
    mcu_family: str | None,
    bus_kinds: tuple[str, ...] = (),
    prompt_version: str = PROMPT_VERSION,
) -> Path:
    return SCOPE_LLM_CACHE_DIR / (
        cache_key_for(
            rtos_id=rtos_id,
            mcu_family=mcu_family,
            bus_kinds=bus_kinds,
            prompt_version=prompt_version,
        )
        + ".json"
    )


# Source-root SHA aggregate


def _aggregate_source_root_sha(source_roots: list[SourceRoot]) -> str:
    """Hash the sorted ``(root_id, sha, str(path))`` tuples so the
    cache key invalidates when any root's git HEAD or registered path
    changes.  We deliberately ignore ``priority`` and ``roles`` because
    the synthesizer doesn't depend on them."""
    pieces = []
    for r in sorted(source_roots, key=lambda x: x.root_id):
        pieces.append(f"{r.root_id}|{r.sha}|{r.path}")
    return hashlib.sha1("\x00".join(pieces).encode("utf-8")).hexdigest()[:16]


# Synthesis


# Some source bundles have one root with multiple roles, while others split
# kernel and platform support into separate roots.
_DEFAULT_GLOBAL_EXCLUDES = (
    "**/.git/**",
    "**/__pycache__/**",
    "**/test/**",
    "**/tests/**",
    "**/demo/**",
    "**/demos/**",
    "**/example/**",
    "**/examples/**",
    "**/sample/**",
    "**/samples/**",
    "**/doc/**",
    "**/docs/**",
    "**/Documentation/**",
    "**/tools/**",
    "**/scripts/**",
    "**/build/**",
    "**/out/**",
)


def _role_label_to_scope_meta(role_name: str) -> tuple[list[str], str, list[str]]:
    """Translate a triage role name to ``(applies_to_roles, role_hint, target_affinity)`` for the scope-map vocabulary."""
    if role_name == "kernel":
        return (["kernel", "runtime"], "kernel", ["mcu_family"])
    if role_name.startswith("driver_framework_"):
        bus = role_name[len("driver_framework_") :]
        return (
            ["driver_framework", "vendor_hal"],
            "driver_framework",
            ["mcu_family", bus],
        )
    return ([role_name], role_name, [])


def _scope_dict_from_fragment(
    fragment: dict,
    *,
    scope_id: str,
    rtos_id: str,
    role_name: str,
) -> dict:
    applies_to_roles, role_hint, target_affinity = _role_label_to_scope_meta(role_name)
    include_dir_patterns: list[str] = []
    seen: set[str] = set()
    for p in (fragment.get("include_dir_patterns") or []):
        if p and p not in seen:
            seen.add(p)
            include_dir_patterns.append(p)
    for p in (fragment.get("include_file_patterns") or []):
        if p and p not in seen:
            seen.add(p)
            include_dir_patterns.append(p)

    exclude_dir_patterns: list[str] = []
    seen.clear()
    for p in (fragment.get("exclude_dir_patterns") or []):
        if p and p not in seen:
            seen.add(p)
            exclude_dir_patterns.append(p)

    return {
        "scope_id": scope_id,
        "applies_to_root_id_patterns": [f"{rtos_id}:*", f"*:{rtos_id}*:*"],
        "applies_to_roles": applies_to_roles,
        "include_dir_patterns": include_dir_patterns,
        "exclude_dir_patterns": exclude_dir_patterns,
        "target_affinity": target_affinity,
        "role_hint": role_hint,
    }


_TRIAGE_ROLES = frozenset(
    {"kernel", "driver_framework", "vendor_hal", "runtime"}
)


def _pick_triage_roots(
    rtos_id: str,
    source_roots: list[SourceRoot],
) -> list[SourceRoot]:
    """Return the SourceRoots whose trees should be triaged by the LLM."""
    if not source_roots:
        return []

    triage = [r for r in source_roots if r.roles & _TRIAGE_ROLES]
    if triage:
        return sorted(triage, key=lambda r: (-r.priority, r.root_id))

    def role_score(r: SourceRoot) -> tuple[int, float]:
        if r.rtos_scope_id == rtos_id:
            return (1, r.priority)
        return (0, r.priority)

    return [max(source_roots, key=role_score)]


def synthesize_scope_map_entry(
    *,
    rtos_id: str,
    source_roots: list[SourceRoot],
    mcu_family: str | None,
    bus_kinds: Iterable[str] = (),
    provider=None,
    budget=None,
    max_rounds: int = 5,
    sample_size: int = 5,
    triage_role_names: list[str] | None = None,
) -> tuple[ScopeMapEntry, dict]:
    """Run LLM triage and assemble a ScopeMapEntry."""
    bus_kinds = _normalise_bus_kinds(bus_kinds)
    if triage_role_names is None:
        triage_role_names = ["kernel"]
        for b in bus_kinds:
            triage_role_names.append(f"driver_framework_{b}")

    triage_roots = _pick_triage_roots(rtos_id, source_roots)
    if not triage_roots:
        raise ValueError(
            f"synthesize_scope_map_entry: no triagable source roots for rtos_id={rtos_id!r}"
        )

    if provider is None:
        # Lazy import to avoid hard-pinning a default model in this module.
        from ...llm.providers import create_provider as _create_provider

        provider = _create_provider("deepseek", model="deepseek-v4-flash")
    # Use a separate budget per root/role pair during cache warm-up.
    use_per_role_budget = budget is None

    # Group unique tree fragments by role and origin root.
    fragments_by_role: dict[str, list[tuple[SourceRoot, dict]]] = {}
    audit_per_root: dict[str, dict[str, dict]] = {}
    seen_paths: dict[Path, str] = {}

    for root in triage_roots:
        root_path = root.path
        if root_path in seen_paths:
            logger.info(
                "scope_llm.synthesize: skipping root %s — same path as %s already triaged",
                root.root_id,
                seen_paths[root_path],
            )
            continue
        seen_paths[root_path] = root.root_id

        for role_name in triage_role_names:
            if role_name == "kernel":
                role = ROLES["kernel"]
            elif role_name.startswith("driver_framework_"):
                bus = role_name[len("driver_framework_") :]
                role = role_for_bus(bus)
            else:
                logger.warning(
                    "synthesize_scope_map_entry: unknown triage role %r — skipping",
                    role_name,
                )
                continue

            logger.info(
                "scope_llm.synthesize: rtos=%s root=%s role=%s mcu=%s bus_kinds=%s",
                rtos_id,
                root.root_id,
                role_name,
                mcu_family or "(none)",
                bus_kinds,
            )
            role_budget = (
                make_budget_tracker(mode="exhaustive")
                if use_per_role_budget
                else budget
            )
            state = run_triage_for_role(
                rtos_root=root_path,
                rtos_id=rtos_id,
                role=role,
                provider=provider,
                budget=role_budget,
                max_rounds=max_rounds,
                sample_size=sample_size,
                mcu_family=mcu_family,
            )
            frag = derive_scope_fragment(state)
            fragments_by_role.setdefault(role_name, []).append((root, frag))
            audit_per_root.setdefault(root.root_id, {})[role_name] = {
                "kept_terminal": sorted(state.kept_terminal),
                "dropped_count": len(state.dropped),
                "kept_self_files": sorted(state.kept_self_files - state.kept_terminal),
                "rounds": [
                    {
                        "round": rec.get("round"),
                        "n_snapshots": rec.get("n_snapshots"),
                        "elapsed_s": rec.get("elapsed_s"),
                        "decisions": rec.get("decisions"),
                    }
                    for rec in state.audit
                ],
            }

    scopes_json: list[dict] = []
    for role_name, root_fragments in fragments_by_role.items():
        for idx, (root, frag) in enumerate(root_fragments):
            scope_id = (
                f"{rtos_id}_{role_name}_{root.root_id.replace(':', '__')}_llm"
                if len(root_fragments) > 1
                else f"{rtos_id}_{role_name}_llm"
            )
            scope_dict = _scope_dict_from_fragment(
                frag,
                scope_id=scope_id,
                rtos_id=rtos_id,
                role_name=role_name,
            )
            # Pin this synthesized scope to the triaged root.
            scope_dict["applies_to_root_id_patterns"] = [root.root_id]
            scopes_json.append(scope_dict)

    raw = {
        "_meta": {
            "description": (
                f"Scope map for rtos_id='{rtos_id}', "
                f"mcu_family='{mcu_family or '*'}'."
            ),
            "version": 1,
            "source": "scope_map_synthesizer",
        },
        "rtos_id": rtos_id,
        "scope_map_version": 1,
        "root_strategy": "llm_synthesised",
        "default_include": [],
        "default_exclude": list(_DEFAULT_GLOBAL_EXCLUDES),
        "scopes": scopes_json,
        "_llm_metadata": {
            "prompt_version": PROMPT_VERSION,
            "mcu_family": mcu_family,
            "bus_kinds": list(bus_kinds),
            "triage_roles": list(triage_role_names),
            "triage_roots": [
                {
                    "root_id": r.root_id,
                    "path": str(r.path),
                    "sha": r.sha,
                    "roles": sorted(r.roles),
                }
                for r in triage_roots
                if r.path in seen_paths
            ],
            "all_source_roots_sha": _aggregate_source_root_sha(source_roots),
            "created_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "audit_summary": audit_per_root,
        },
    }

    entry = _build_entry(rtos_id, raw)
    return entry, raw


# Disk cache I/O


def _load_cache(path: Path) -> dict | None:
    if not path.exists():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as exc:
        logger.warning("scope_llm cache read failed for %s: %s", path, exc)
        return None


def _is_cache_valid(
    cached: dict,
    *,
    expected_root_sha: str,
    expected_prompt_version: str,
) -> bool:
    md = cached.get("_llm_metadata") or {}
    if md.get("prompt_version") != expected_prompt_version:
        return False
    if md.get("all_source_roots_sha") != expected_root_sha:
        return False
    return True


def _save_cache(path: Path, raw: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(raw, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )


# Public API


def load_or_synthesize_scope_map(
    *,
    rtos_id: str,
    source_roots: list[SourceRoot],
    mcu_family: str | None = None,
    bus_kinds: Iterable[str] = (),
    mode: str | None = None,
    provider=None,
    budget=None,
    max_rounds: int = 5,
    sample_size: int = 5,
) -> ScopeMapEntry | None:
    """Resolve a ScopeMapEntry for the given task."""
    bus_kinds = _normalise_bus_kinds(bus_kinds)
    eff_mode = _resolve_mode(mode)

    if eff_mode == "off":
        # No scope_llm. Caller indexes without role hints.
        return None

    cache_file = cache_path_for(
        rtos_id=rtos_id,
        mcu_family=mcu_family,
        bus_kinds=bus_kinds,
    )
    expected_root_sha = _aggregate_source_root_sha(source_roots)

    if eff_mode in ("auto", "cache_only"):
        cached = _load_cache(cache_file)
        if cached is not None and _is_cache_valid(
            cached,
            expected_root_sha=expected_root_sha,
            expected_prompt_version=PROMPT_VERSION,
        ):
            logger.info(
                "scope_llm: cache HIT for %s (mcu=%s, buses=%s)",
                rtos_id,
                mcu_family,
                bus_kinds,
            )
            return _build_entry(rtos_id, cached)
        if cached is not None:
            logger.info(
                "scope_llm: cache STALE for %s (root_sha or prompt_version mismatch)",
                rtos_id,
            )
        if eff_mode == "cache_only":
            logger.info(
                "scope_llm: cache_only mode and no fresh cache for %s — "
                "returning None (no static fallback)",
                rtos_id,
            )
            return None

    # Auto miss or force mode runs live triage.
    try:
        entry, raw = synthesize_scope_map_entry(
            rtos_id=rtos_id,
            source_roots=source_roots,
            mcu_family=mcu_family,
            bus_kinds=bus_kinds,
            provider=provider,
            budget=budget,
            max_rounds=max_rounds,
            sample_size=sample_size,
        )
    except Exception as exc:  # noqa: BLE001 — broad fallback for resilience
        # Fall back to unscoped indexing when live triage fails.
        logger.exception(
            "scope_llm: triage failed for rtos_id=%s mcu=%s buses=%s: %s — "
            "returning None (no static JSON fallback)",
            rtos_id,
            mcu_family,
            bus_kinds,
            exc,
        )
        return None

    try:
        _save_cache(cache_file, raw)
        logger.info("scope_llm: cached synthesised scope map at %s", cache_file)
    except OSError as exc:
        logger.warning("scope_llm: failed to write cache %s: %s", cache_file, exc)

    return entry


__all__ = [
    "SCOPE_LLM_CACHE_DIR",
    "cache_key_for",
    "cache_path_for",
    "synthesize_scope_map_entry",
    "load_or_synthesize_scope_map",
]
