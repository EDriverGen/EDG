"""Transaction-coverage check between derived and planned transactions."""
from __future__ import annotations

import dataclasses
import logging
from typing import Any, Dict, FrozenSet, List, Mapping, Optional, Sequence, Tuple

from .ir_to_expected_transactions import ExpectedTransaction

logger = logging.getLogger(__name__)


# Data classes

@dataclasses.dataclass(frozen=True)
class CoverageMatch:
    """Pairs a derived transaction with its planned counterpart."""
    mechanical: ExpectedTransaction
    llm_match: Optional[Dict[str, Any]]
    reason: str

    def to_dict(self) -> Dict[str, Any]:
        return {
            "mechanical": self.mechanical.to_dict(),
            "llm_match":  self.llm_match,
            "reason":     self.reason,
            "covered":    self.llm_match is not None,
        }


@dataclasses.dataclass(frozen=True)
class CoverageReport:
    """Result of cross-checking planned transactions against derived ones."""
    covered: Tuple[CoverageMatch, ...]
    missing: Tuple[CoverageMatch, ...]
    llm_extras: Tuple[Dict[str, Any], ...]
    merged: Tuple[Dict[str, Any], ...]
    summary: str

    @property
    def ok(self) -> bool:
        return not self.missing

    @property
    def superset(self) -> bool:
        """Alias for :attr:`ok`."""
        return self.ok

    def to_dict(self) -> Dict[str, Any]:
        return {
            "ok":                self.ok,
            "superset":          self.superset,
            "covered_count":     len(self.covered),
            "missing_count":     len(self.missing),
            "llm_extras_count":  len(self.llm_extras),
            "merged_count":      len(self.merged),
            "summary":           self.summary,
            "covered":           [m.to_dict() for m in self.covered],
            "missing":           [m.to_dict() for m in self.missing],
            "llm_extras":        list(self.llm_extras),
            "merged":            list(self.merged),
        }


# Canonicalization helpers

def _canon_hex(token: Any) -> str:
    """Canonicalise a hex literal so comparisons are case/zero-padding safe."""
    if isinstance(token, int):
        if 0 <= token <= 0xFFFF:
            width = 2 if token <= 0xFF else 4
            return f"0x{token:0{width}X}"
        return str(token)
    if isinstance(token, str):
        s = token.strip()
        if s.lower().startswith("0x"):
            try:
                n = int(s, 16)
            except ValueError:
                return s
            width = 2 if n <= 0xFF else 4
            return f"0x{n:0{width}X}"
        return s
    return str(token)


def _canon_prefix(raw: Any) -> Tuple[str, ...]:
    """Normalise one prefix option into a tuple of canonical hex strings."""
    if raw is None:
        return ()
    if isinstance(raw, (list, tuple)):
        return tuple(_canon_hex(x) for x in raw)
    # Tolerate scalar options inside write_prefix_any_of.
    return (_canon_hex(raw),)


def _canon_prefix_set(raw: Any) -> FrozenSet[Tuple[str, ...]]:
    """Normalise ``write_prefix_any_of`` to a canonical frozen set."""
    if raw is None:
        return frozenset()
    if not isinstance(raw, (list, tuple)):
        return frozenset({_canon_prefix(raw)})
    out = set()
    for opt in raw:
        canon = _canon_prefix(opt)
        if canon:
            out.add(canon)
    return frozenset(out)


def _llm_tx_key(llm: Mapping[str, Any]) -> Tuple[str, str, FrozenSet[Tuple[str, ...]]]:
    """Identity key for a planned transaction used for dedup in ``merged``."""
    return (
        str(llm.get("phase", "")),
        str(llm.get("addr_or_pin", "") or ""),
        _canon_prefix_set(llm.get("write_prefix_any_of")),
    )


def _mech_as_dict(m: ExpectedTransaction) -> Dict[str, Any]:
    """Convert a derived ExpectedTransaction to the planned dict shape."""
    return m.to_dict()


# Matching

def _matches(
    mech: ExpectedTransaction,
    llm: Mapping[str, Any],
) -> Tuple[bool, str]:
    """Return ``(covered, reason)`` for one derived vs planned pair."""
    llm_phase = str(llm.get("phase", ""))
    if llm_phase != mech.phase:
        return False, f"phase mismatch ({llm_phase!r} vs {mech.phase!r})"

    mech_addr = (
        "" if mech.addr_or_pin is None else str(mech.addr_or_pin)
    ).strip()
    llm_addr = str(llm.get("addr_or_pin", "") or "").strip()
    if mech_addr and llm_addr and mech_addr != llm_addr:
        return False, (f"addr mismatch ({llm_addr!r} vs {mech_addr!r})")

    mech_set = frozenset(
        tuple(_canon_hex(x) for x in opt)
        for opt in mech.write_prefix_any_of
    )
    llm_set = _canon_prefix_set(llm.get("write_prefix_any_of"))

    if mech.forbid_write_prefix and llm_set:
        return False, (
            "mechanical transaction requires a direct read with no preceding "
            f"write prefix, but plan declares prefixes {sorted(llm_set)}"
        )

    # Empty-both read-any marker matched on phase+addr alone.
    if not mech_set and not llm_set:
        return True, "read-any match (no prefixes required)"
    if not mech_set:
        return True, "derived row is read-any; plan adds specific prefix"
    # Derived row demands a prefix but the plan claims none.
    if not llm_set:
        return False, (
            f"plan provides no write_prefix_any_of on phase={mech.phase} "
            f"addr={mech.addr_or_pin!r} while mechanical requires one of "
            f"{sorted(mech_set)}"
        )

    overlap = mech_set & llm_set
    if overlap:
        return True, (f"prefix overlap: {sorted(overlap)} ∈ mechanical "
                      f"{sorted(mech_set)}")

    # Planned prefixes that do not overlap the derived set are not covered.
    return False, (
        f"no prefix overlap; plan={sorted(llm_set)} vs "
        f"mechanical={sorted(mech_set)} is empty"
    )


def _find_first_match(
    mech: ExpectedTransaction,
    llm_list: Sequence[Mapping[str, Any]],
) -> Tuple[Optional[Mapping[str, Any]], str]:
    """Return the first planned entry that covers ``mech``."""
    last_reason = "no planned transaction on this phase+addr"
    for llm in llm_list:
        if not isinstance(llm, Mapping):
            continue
        covered, reason = _matches(mech, llm)
        if covered:
            return llm, reason
        # Keep the closest miss for diagnostics.
        if str(llm.get("phase", "")) == mech.phase:
            last_reason = reason
    return None, last_reason


# Public API

def check_transaction_coverage(
    llm_transactions: Sequence[Mapping[str, Any]],
    mechanical: Sequence[ExpectedTransaction],
) -> CoverageReport:
    """Check that ``llm_transactions`` cover every ``mechanical`` entry."""
    mech_list = [m for m in mechanical if isinstance(m, ExpectedTransaction)]
    llm_list: List[Mapping[str, Any]] = [
        dict(t) for t in llm_transactions if isinstance(t, Mapping)
    ]

    covered: List[CoverageMatch] = []
    missing: List[CoverageMatch] = []

    # Track planned entries explained by at least one derived entry.
    llm_covering: set = set()

    for mech in mech_list:
        llm_match, reason = _find_first_match(mech, llm_list)
        cm = CoverageMatch(
            mechanical=mech,
            llm_match=dict(llm_match) if llm_match is not None else None,
            reason=reason,
        )
        if llm_match is None:
            missing.append(cm)
        else:
            covered.append(cm)
            # Record by id() since Mapping isn't hashable in general.
            llm_covering.add(id(llm_match))

    llm_extras: List[Dict[str, Any]] = []
    seen_keys: set = set()
    for llm in llm_list:
        if id(llm) in llm_covering:
            continue
        # Deduplicate structurally identical extras.
        key = _llm_tx_key(llm)
        if key in seen_keys:
            continue
        seen_keys.add(key)
        llm_extras.append(dict(llm))

    # Merge planned entries first, then any missing derived entries.
    merged: List[Dict[str, Any]] = []
    merge_seen: set = set()
    for llm in llm_list:
        k = _llm_tx_key(llm)
        if k in merge_seen:
            continue
        merge_seen.add(k)
        merged.append(dict(llm))
    for mech in mech_list:
        d = _mech_as_dict(mech)
        k = _llm_tx_key(d)
        if k in merge_seen:
            continue
        merge_seen.add(k)
        merged.append(d)

    parts: List[str] = []
    if mech_list:
        parts.append(f"{len(covered)}/{len(mech_list)} mechanical covered")
    else:
        parts.append("no mechanical transactions derived")
    if missing:
        parts.append(f"{len(missing)} missing")
    if llm_extras:
        parts.append(f"{len(llm_extras)} planned extras")
    summary = "; ".join(parts)

    logger.info("merge_test_plan: %s", summary)
    return CoverageReport(
        covered=tuple(covered),
        missing=tuple(missing),
        llm_extras=tuple(llm_extras),
        merged=tuple(merged),
        summary=summary,
    )


def check_bundle_coverage(
    test_plan: Mapping[str, Any],
    mechanical: Sequence[ExpectedTransaction],
) -> CoverageReport:
    """Convenience: pull ``expected_transactions`` off a test_plan dict."""
    llm_txs_raw = test_plan.get("expected_transactions") or []
    if not isinstance(llm_txs_raw, (list, tuple)):
        llm_txs_raw = []
    return check_transaction_coverage(llm_txs_raw, mechanical)


__all__ = [
    "CoverageMatch",
    "CoverageReport",
    "check_transaction_coverage",
    "check_bundle_coverage",
]
