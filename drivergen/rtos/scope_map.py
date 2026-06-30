"""pipeline step ScopeMap parser, applier, and consistency check."""

from __future__ import annotations

import copy
import fnmatch
from dataclasses import dataclass, field

from .types import SourceRoot


_VALID_ROLES = frozenset(
    {
        "kernel",
        "runtime",
        "driver_framework",
        "vendor_hal",
        "board_integration",
        "docs",
        "exemplar",
    }
)

_VALID_ROLE_HINTS = frozenset(
    {
        "kernel",
        "runtime",
        "vendor_hal",
        "driver_framework",
        "board",
        "docs",
        "exemplar",
    }
)


@dataclass(frozen=True)
class ScopeRule:
    """One ``scopes[]`` entry; see module docstring for schema."""

    scope_id: str
    applies_to_root_id_patterns: tuple[str, ...] = ()
    applies_to_roles: frozenset[str] = field(default_factory=frozenset)
    include_dir_patterns: tuple[str, ...] = ()
    exclude_dir_patterns: tuple[str, ...] = ()
    target_affinity: tuple[str, ...] = ()
    role_hint: str | None = None


@dataclass
class ScopeMapEntry:
    """Parsed contents of one generated ScopeMap."""

    rtos_id: str
    scope_map_version: int
    root_strategy: str
    default_include: tuple[str, ...]
    default_exclude: tuple[str, ...]
    scopes: list[ScopeRule]
    raw: dict
    """Original JSON, kept for reproducibility / hashing."""

    def scopes_for_root(self, root: SourceRoot) -> list[ScopeRule]:
        """Return the ordered list of scopes that apply to *root*."""
        return [s for s in self.scopes if _scope_matches_root(s, root)]


def _build_entry(rtos_id: str, raw: dict) -> ScopeMapEntry:
    """Parse a raw ScopeMap dictionary into a runtime entry."""
    if "scopes" not in raw or not isinstance(raw["scopes"], list):
        raise ValueError(
            f"ScopeMap for '{rtos_id}' is missing a top-level 'scopes' list"
        )
    if raw.get("rtos_id") and raw["rtos_id"] != rtos_id:
        raise ValueError(
            f"ScopeMap key '{rtos_id}' does not match its rtos_id field "
            f"'{raw['rtos_id']}'"
        )

    scopes: list[ScopeRule] = []
    seen_ids: set[str] = set()
    for idx, raw_scope in enumerate(raw["scopes"]):
        if not isinstance(raw_scope, dict):
            raise ValueError(
                f"ScopeMap '{rtos_id}': scopes[{idx}] is not an object"
            )
        scope_id = raw_scope.get("scope_id") or f"scope_{idx}"
        if scope_id in seen_ids:
            raise ValueError(
                f"ScopeMap '{rtos_id}': duplicate scope_id '{scope_id}'"
            )
        seen_ids.add(scope_id)
        scopes.append(
            ScopeRule(
                scope_id=scope_id,
                applies_to_root_id_patterns=tuple(
                    raw_scope.get("applies_to_root_id_patterns") or []
                ),
                applies_to_roles=frozenset(raw_scope.get("applies_to_roles") or []),
                include_dir_patterns=tuple(
                    raw_scope.get("include_dir_patterns") or []
                ),
                exclude_dir_patterns=tuple(
                    raw_scope.get("exclude_dir_patterns") or []
                ),
                target_affinity=tuple(raw_scope.get("target_affinity") or []),
                role_hint=raw_scope.get("role_hint"),
            )
        )

    return ScopeMapEntry(
        rtos_id=raw.get("rtos_id") or rtos_id,
        scope_map_version=int(raw.get("scope_map_version") or 0),
        root_strategy=str(raw.get("root_strategy") or "unknown"),
        default_include=tuple(raw.get("default_include") or []),
        default_exclude=tuple(raw.get("default_exclude") or []),
        scopes=scopes,
        # Keep raw diagnostics independent from the cache payload.
        raw=copy.deepcopy(raw),
    )


def _scope_matches_root(scope: ScopeRule, root: SourceRoot) -> bool:
    """Return true iff *scope* applies to *root*."""
    have_id_filter = bool(scope.applies_to_root_id_patterns)
    have_role_filter = bool(scope.applies_to_roles)
    if not have_id_filter and not have_role_filter:
        return False

    if have_id_filter and not any(
        fnmatch.fnmatchcase(root.root_id, p)
        for p in scope.applies_to_root_id_patterns
    ):
        return False
    if have_role_filter and not (root.roles & scope.applies_to_roles):
        return False
    return True


def _normalise_rel_path(rel_path: str) -> str:
    """Coerce *rel_path* to a forward-slash, no-leading-``./`` form."""
    norm = rel_path.replace("\\", "/")
    while norm.startswith("./"):
        norm = norm[2:]
    while norm.startswith("/"):
        norm = norm[1:]
    return norm


def _matches_any(rel_path: str, patterns: tuple[str, ...]) -> bool:
    if not patterns:
        return False
    for p in patterns:
        if fnmatch.fnmatchcase(rel_path, p):
            return True
    return False


def path_in_scope(rel_path: str, scope: ScopeRule) -> bool:
    """Return true when *rel_path* is included and not excluded by *scope*."""
    norm = _normalise_rel_path(rel_path)
    included = (
        True
        if not scope.include_dir_patterns
        else _matches_any(norm, scope.include_dir_patterns)
    )
    if not included:
        return False
    if _matches_any(norm, scope.exclude_dir_patterns):
        return False
    return True


def assign_dir_role(scopes_for_root: list[ScopeRule], rel_path: str) -> str | None:
    """Return the first matching scope's ``role_hint`` for *rel_path*."""
    for scope in scopes_for_root:
        if path_in_scope(rel_path, scope):
            return scope.role_hint
    return None


@dataclass
class ScopeMapValidationReport:
    rtos_id: str
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return not self.errors


def validate_scope_map_against_roots(
    scope_map: ScopeMapEntry,
    source_roots: list[SourceRoot],
    *,
    strict_unmatched_root_id_patterns: bool = False,
) -> ScopeMapValidationReport:
    """Sanity-check a ScopeMap against the active SourceRoot list."""
    report = ScopeMapValidationReport(rtos_id=scope_map.rtos_id)

    for scope in scope_map.scopes:
        if scope.role_hint and scope.role_hint not in _VALID_ROLE_HINTS:
            report.errors.append(
                f"scope '{scope.scope_id}': role_hint='{scope.role_hint}' "
                f"is not in the valid vocabulary {sorted(_VALID_ROLE_HINTS)}"
            )
        bad_roles = scope.applies_to_roles - _VALID_ROLES
        if bad_roles:
            report.errors.append(
                f"scope '{scope.scope_id}': applies_to_roles contains unknown "
                f"entries {sorted(bad_roles)}; valid roles are {sorted(_VALID_ROLES)}"
            )

    for scope in scope_map.scopes:
        for pat in scope.applies_to_root_id_patterns:
            if not any(
                fnmatch.fnmatchcase(r.root_id, pat) for r in source_roots
            ):
                msg = (
                    f"scope '{scope.scope_id}': applies_to_root_id_patterns "
                    f"'{pat}' matches none of the {len(source_roots)} active "
                    f"source roots"
                )
                if strict_unmatched_root_id_patterns:
                    report.errors.append(msg)
                else:
                    report.warnings.append(msg)

    matched_root_ids = set()
    for scope in scope_map.scopes:
        for r in source_roots:
            if _scope_matches_root(scope, r):
                matched_root_ids.add(r.root_id)
    for r in source_roots:
        if r.root_id not in matched_root_ids:
            report.warnings.append(
                f"SourceRoot '{r.root_id}' (roles={sorted(r.roles)}) is not "
                f"covered by any scope; it will use default include/exclude "
                f"only and produce no dir_role_hint."
            )

    return report


__all__ = [
    "ScopeRule",
    "ScopeMapEntry",
    "ScopeMapValidationReport",
    "path_in_scope",
    "assign_dir_role",
    "validate_scope_map_against_roots",
]
