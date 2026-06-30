"""Cross-round invariants tracker for the repair loop."""
from __future__ import annotations

import dataclasses
import re
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

from .consistency_check import (
    VERDICT_L1,
    VERDICT_L2,
    ConsistencyReport,
)
from .merge_test_plan import CoverageReport
from .runtime_probe import (
    ProbeOutcome,
    ProbeStimulus,
    check_probe_expectations,
)


# Stable invariant kind tags.
KIND_BYTE_ORDER = "byte-order"
KIND_RAW_WIDTH = "raw-width"
KIND_SIGN = "sign"
KIND_INIT_OPCODE = "init-opcode"
KIND_READ_ADDRESS = "read-address"
KIND_SPI_MASK = "spi-mask"
KIND_UART_FRAMING = "uart-framing"
KIND_GPIO_TIMING = "gpio-timing"
KIND_ADDRESS_SIZE = "address-size"
KIND_RUNTIME_PASS = "runtime-pass"
# Sticky include invariant for syntax-clean attempts.
KIND_COMPILE_INCLUDES = "compile-includes"


# Recognized interpretation tags emitted by consistency checks.
_BYTE_ORDER_EVIDENCE_RE = re.compile(
    r"\b("
    r"big_endian_u32|little_endian_u32|"
    r"big_endian_u24|little_endian_u24|"
    r"big_endian_u16|little_endian_u16|"
    r"big_endian_i32|little_endian_i32|"
    r"big_endian_i24|little_endian_i24|"
    r"big_endian_i16|little_endian_i16|"
    r"last_u8|first_u8|last_i8|first_i8"
    r")\b"
)


@dataclasses.dataclass(frozen=True)
class Invariant:
    """A single confirmed fact about the device."""
    kind: str
    statement: str
    source_attempt: int
    evidence: str

    @property
    def key(self) -> Tuple[str, str]:
        return (self.kind, self.statement)

    def to_bullet(self) -> str:
        """Format as a single markdown bullet."""
        ev = f" (attempt {self.source_attempt}"
        if self.evidence:
            ev += f", evidence: {self.evidence}"
        ev += ")"
        return f"- [{self.kind}] {self.statement}{ev}"

    def to_dict(self) -> Dict[str, Any]:
        return {
            "kind":           self.kind,
            "statement":      self.statement,
            "source_attempt": self.source_attempt,
            "evidence":       self.evidence,
        }


# Mining helpers

def _byte_order_kind_from_interp(interp_tag: str) -> Optional[str]:
    """Translate an interpretation tag into a human phrase."""
    if not interp_tag:
        return None
    # Convert canonical snake_case into a readable phrase.
    parts = interp_tag.replace("_", " ").split()
    if len(parts) < 2:
        return None
    # Preserve width and signedness token at the end.
    return " ".join(parts)


def _extract_mock_bytes_from_evidence(evidence: str) -> Optional[str]:
    """Pull the ``bytes=[0x04, 0xB0]`` fragment from an L1 evidence string."""
    m = re.search(r"bytes\s*[:=]?\s*(\[[^\]]+\])", evidence)
    return m.group(1) if m else None


def _invariants_from_consistency(
    attempt: int, report: ConsistencyReport,
) -> List[Invariant]:
    """Promote consistency successes to invariants."""
    out: List[Invariant] = []
    for stim in report.stimuli:
        if stim.verdict == VERDICT_L1:
            tag = _BYTE_ORDER_EVIDENCE_RE.search(stim.evidence or "")
            if tag:
                interp = tag.group(1)
                phrase = _byte_order_kind_from_interp(interp) or interp
                bytes_hint = _extract_mock_bytes_from_evidence(stim.evidence or "")
                evidence = (f"stim `{stim.name}` matched {interp}"
                            + (f" on {bytes_hint}" if bytes_hint else ""))
                out.append(Invariant(
                    kind=KIND_BYTE_ORDER,
                    statement=f"raw reads decode as {phrase}",
                    source_attempt=attempt,
                    evidence=evidence,
                ))
        # Unproven or inconsistent results do not become invariants.
    return out


def _invariants_from_coverage(
    attempt: int, report: CoverageReport,
) -> List[Invariant]:
    """Promote covered matches to init/read invariants."""
    out: List[Invariant] = []
    for match in report.covered:
        et = match.mechanical
        phase = (et.phase or "").lower()
        addr = et.addr_or_pin or "?"
        prefixes = et.write_prefix_any_of
        # Render prefix set in short form: prefer the first option.
        if prefixes:
            first = prefixes[0]
            prefix_str = "[" + ", ".join(str(x) for x in first) + "]"
            if len(prefixes) > 1:
                prefix_str += f" (+{len(prefixes) - 1} alt)"
        elif et.read_any:
            prefix_str = "<read>"
        else:
            prefix_str = "<empty>"

        if phase == "init":
            kind = KIND_INIT_OPCODE
            statement = (
                f"init phase must write {prefix_str} to addr {addr}"
            )
        elif phase == "read":
            kind = KIND_READ_ADDRESS
            statement = (
                f"read phase uses addr {addr} with "
                f"prefix {prefix_str}"
            )
        else:
            # Fall back to a generic write obligation.
            kind = KIND_INIT_OPCODE
            statement = (
                f"phase={phase} requires write {prefix_str} "
                f"to addr {addr}"
            )
        out.append(Invariant(
            kind=kind,
            statement=statement,
            source_attempt=attempt,
            evidence=et.source or et.note or "mechanical+llm agreement",
        ))
    return out


_INCLUDE_RE = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*([<"])([^>"\n]+)[>"]',
    re.MULTILINE,
)


def _extract_includes_from_text(text: str) -> List[Tuple[str, str]]:
    """Pull every ``#include`` directive from a C source/header."""
    if not text:
        return []
    out: List[Tuple[str, str]] = []
    for m in _INCLUDE_RE.finditer(text):
        framing = m.group(1)
        header = m.group(2).strip()
        if header:
            out.append((framing, header))
    return out


def _invariants_from_compile_clean_includes(
    attempt: int,
    *,
    header_text: str,
    source_text: str,
) -> List[Invariant]:
    """Promote every `#include` in a compile-clean attempt to a keep-invariant."""
    seen: set[Tuple[str, str]] = set()
    out: List[Invariant] = []
    for framing, header in (
        _extract_includes_from_text(header_text)
        + _extract_includes_from_text(source_text)
    ):
        key = (framing, header)
        if key in seen:
            continue
        seen.add(key)
        spelt = f"<{header}>" if framing == "<" else f'"{header}"'
        out.append(Invariant(
            kind=KIND_COMPILE_INCLUDES,
            statement=(
                f"`#include {spelt}` resolved cleanly under stub_compile; "
                "KEEP this exact include (do NOT switch to a different "
                "vendor / cross-family / LL header in the next attempt)"
            ),
            source_attempt=attempt,
            evidence="L1 syntax compile passed for driver_header+driver_source",
        ))
    return out


def _invariants_from_probe(
    attempt: int,
    outcomes: Sequence[ProbeOutcome],
    stimuli: Sequence[ProbeStimulus],
) -> List[Invariant]:
    """Promote passing runtime stimuli to runtime-pass invariants."""
    out: List[Invariant] = []
    stim_map = {s.name: s for s in (stimuli or [])}
    for o in outcomes:
        if o.error:
            continue
        if not o.boot_detected or not o.test_done or not o.result_pass:
            continue
        stim = stim_map.get(o.stimulus_name)
        if not check_probe_expectations(o, stim).ok:
            continue
        parts: List[str] = [f"stim `{o.stimulus_name}` passed on Renode"]
        if stim is not None:
            if stim.expected_read_raw is not None:
                parts.append(f"at raw={stim.expected_read_raw:g}")
            elif stim.expected_mem_bytes:
                parts.append("(memory match)")
            elif stim.expected_channels:
                parts.append("(multi_channel match)")
            elif stim.expected_time:
                parts.append("(RTC match)")
        statement = " ".join(parts)
        out.append(Invariant(
            kind=KIND_RUNTIME_PASS,
            statement=statement,
            source_attempt=attempt,
            evidence=f"runtime result_pass=True, test_done=True",
        ))
    return out


# Tracker

class InvariantsTracker:
    """Mutable accumulator of :class:`Invariant` across repair rounds."""

    def __init__(self) -> None:
        # Keep a dict keyed by Invariant.key for O(1) dedup. Values are
        # the Invariant with the EARLIEST source_attempt.
        self._facts: Dict[Tuple[str, str], Invariant] = {}

    # Internal add / dedup

    def _add(self, inv: Invariant) -> bool:
        """Add one invariant. Returns True iff it's new or replaced an
        older one with a later attempt (the latter shouldn't happen if
        callers use monotonic attempt numbers, but we're defensive).
        """
        existing = self._facts.get(inv.key)
        if existing is None:
            self._facts[inv.key] = inv
            return True
        if inv.source_attempt < existing.source_attempt:
            # Shouldn't happen; ingest order goes forward in time.
            self._facts[inv.key] = inv
            return True
        return False

    # Public ingest API

    def ingest_consistency(
        self, attempt: int, report: Optional[ConsistencyReport],
    ) -> int:
        """Mine consistency reports. Returns the count added."""
        if report is None:
            return 0
        added = 0
        for inv in _invariants_from_consistency(attempt, report):
            if self._add(inv):
                added += 1
        return added

    def ingest_coverage(
        self, attempt: int, report: Optional[CoverageReport],
    ) -> int:
        """Mine coverage reports. Returns the count added."""
        if report is None:
            return 0
        added = 0
        for inv in _invariants_from_coverage(attempt, report):
            if self._add(inv):
                added += 1
        return added

    def ingest_compile_clean_includes(
        self,
        attempt: int,
        *,
        header_text: str,
        source_text: str,
    ) -> int:
        """Mine `#include` directives from a syntax-clean attempt."""
        added = 0
        for inv in _invariants_from_compile_clean_includes(
            attempt,
            header_text=header_text or "",
            source_text=source_text or "",
        ):
            if self._add(inv):
                added += 1
        return added

    def ingest_probe(
        self,
        attempt: int,
        outcomes: Optional[Sequence[ProbeOutcome]],
        stimuli: Optional[Sequence[ProbeStimulus]] = None,
    ) -> int:
        """Mine runtime probe outcomes."""
        if not outcomes:
            return 0
        added = 0
        for inv in _invariants_from_probe(attempt, outcomes, stimuli or ()):
            if self._add(inv):
                added += 1
        return added

    def add_manual(
        self,
        kind: str,
        statement: str,
        source_attempt: int,
        evidence: str = "",
    ) -> bool:
        """Insert a manually-curated invariant."""
        return self._add(Invariant(
            kind=kind,
            statement=statement,
            source_attempt=source_attempt,
            evidence=evidence,
        ))

    # Public query API

    def active(self) -> Tuple[Invariant, ...]:
        """Return all currently-tracked invariants."""
        return tuple(sorted(
            self._facts.values(),
            key=lambda i: (i.source_attempt, i.kind, i.statement),
        ))

    def count(self) -> int:
        return len(self._facts)

    def is_empty(self) -> bool:
        return not self._facts

    def to_feedback(self) -> Tuple[str, ...]:
        """Render every invariant as a bullet string."""
        return tuple(inv.to_bullet() for inv in self.active())

    def to_dict(self) -> Dict[str, Any]:
        """Snapshot for JSON logs."""
        return {
            "count":      self.count(),
            "invariants": [inv.to_dict() for inv in self.active()],
        }

    def snapshot(self) -> Tuple[Invariant, ...]:
        """Alias for :meth:`active`; returns an immutable tuple."""
        return self.active()

    # Contradiction detection

    def contradictions(
        self, latest_probe_outcomes: Optional[Sequence[ProbeOutcome]] = None,
    ) -> Tuple[str, ...]:
        """Detect cases where the current attempt violates an invariant."""
        if not latest_probe_outcomes:
            return ()
        warnings: List[str] = []
        outcome_map = {o.stimulus_name: o for o in latest_probe_outcomes}
        for inv in self.active():
            if inv.kind != KIND_RUNTIME_PASS:
                continue
            # Statement format: "stim `NAME` passed on Renode ..."
            m = re.match(r"stim `([^`]+)`", inv.statement)
            if not m:
                continue
            name = m.group(1)
            o = outcome_map.get(name)
            if o is None:
                continue
            if o.error:
                continue
            if not (o.boot_detected and o.test_done and o.result_pass):
                warnings.append(
                    f"[regression] stim `{name}` passed earlier "
                    f"(attempt {inv.source_attempt}) but failed in the "
                    "current attempt; do not introduce changes that "
                    "break a stimulus that was already working"
                )
        return tuple(warnings)


__all__ = [
    "KIND_BYTE_ORDER",
    "KIND_RAW_WIDTH",
    "KIND_SIGN",
    "KIND_INIT_OPCODE",
    "KIND_READ_ADDRESS",
    "KIND_SPI_MASK",
    "KIND_UART_FRAMING",
    "KIND_GPIO_TIMING",
    "KIND_ADDRESS_SIZE",
    "KIND_RUNTIME_PASS",
    "KIND_COMPILE_INCLUDES",
    "Invariant",
    "InvariantsTracker",
]
