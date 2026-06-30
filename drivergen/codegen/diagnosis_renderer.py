"""Diagnosis renderer for repair-loop feedback."""
from __future__ import annotations

import dataclasses
import re
from typing import Any, List, Mapping, Optional, Sequence, Tuple

from .consistency_check import (
    VERDICT_INCONSISTENT,
    VERDICT_LLM_ONLY,
    ConsistencyReport,
)
from .merge_test_plan import CoverageReport
from .runtime_probe import (
    ProbeOutcome,
    ProbeStimulus,
    check_probe_expectations,
)
from .code_generator import SynthesisError


# Severity tiers drive final rendering order + rollup detection.
SEVERITY_ERROR = "error"
SEVERITY_WARNING = "warning"
SEVERITY_INFO = "info"

# Keep feedback compact enough for the next repair prompt.
MAX_PROBE_FAILURES_SHOWN = 6
MAX_COVERAGE_MISSING_SHOWN = 8
MAX_COVERAGE_EXTRAS_SHOWN = 6
MAX_CONSISTENCY_ISSUES_SHOWN = 6
MAX_SCHEMA_ERRORS_SHOWN = 8
MAX_RAW_RESPONSE_CHARS = 800


# Dataclasses

@dataclasses.dataclass(frozen=True)
class DiagnosisSection:
    """One section of the feedback block."""
    heading: str
    severity: str
    lines: Tuple[str, ...]

    def is_empty(self) -> bool:
        return not self.lines


@dataclasses.dataclass(frozen=True)
class DiagnosisReport:
    """Aggregated diagnosis for one attempt, ready to render."""
    attempt: int
    sections: Tuple[DiagnosisSection, ...] = ()

    @property
    def has_errors(self) -> bool:
        return any(s.severity == SEVERITY_ERROR and s.lines for s in self.sections)

    @property
    def has_warnings(self) -> bool:
        return any(
            s.severity == SEVERITY_WARNING and s.lines for s in self.sections
        )

    @property
    def is_empty(self) -> bool:
        """True when there is nothing useful to show the next attempt."""
        return not any(s.lines for s in self.sections)

    def render(self) -> str:
        """Assemble the markdown feedback block."""
        if self.is_empty:
            return ""
        out: List[str] = []
        if self.attempt > 0:
            out.append(f"## ATTEMPT {self.attempt} FEEDBACK")
            out.append("")
        for sec in self.sections:
            if sec.is_empty():
                continue
            out.append(f"### {sec.heading}")
            out.extend(sec.lines)
            out.append("")
        return "\n".join(out).rstrip() + "\n"

    def to_dict(self) -> dict:
        """JSON-friendly snapshot for logs."""
        return {
            "attempt": self.attempt,
            "sections": [
                {"heading": s.heading, "severity": s.severity,
                 "lines": list(s.lines)}
                for s in self.sections
            ],
        }


# Synthesis errors

def _truncate_raw_response(raw: Optional[str]) -> str:
    """Shrink a raw model response for embedding in feedback."""
    if not raw:
        return "(empty response)"
    raw = raw.strip()
    if not raw:
        # whitespace-only input strips to empty; we want the caller to
        # see "(empty response)" rather than a blank line in the feedback
        return "(empty response)"
    if len(raw) <= MAX_RAW_RESPONSE_CHARS:
        return raw
    head = raw[:400]
    tail = raw[-200:]
    return f"{head}\n... [{len(raw) - 600} chars omitted] ...\n{tail}"


def render_synthesis_error(err: Optional[SynthesisError]) -> DiagnosisSection:
    """Translate a :class:`SynthesisError` into actionable feedback."""
    if err is None:
        return DiagnosisSection(
            heading="Synthesis errors", severity=SEVERITY_INFO, lines=(),
        )
    lines: List[str] = []
    source = getattr(err, "source", "unknown")
    raw_response = getattr(err, "raw_response", None)
    errs: Sequence[str] = getattr(err, "errors", ()) or ()

    if source == "provider":
        lines.append(
            f"- provider call failed: {err}. repair_loop will retry "
            "with the same prompt; no driver change is needed."
        )
        return DiagnosisSection(
            heading="Synthesis errors", severity=SEVERITY_INFO,
            lines=tuple(lines),
        )

    if source == "parse":
        lines.append(
            "- the next response must be a single **bare JSON object** "
            "starting with `{` and ending with `}`; no markdown fences, "
            "no prose before or after the braces, no trailing comments. "
            "The pipeline reads the first non-whitespace character; "
            "anything other than `{` is rejected immediately."
        )
        snippet = _truncate_raw_response(raw_response)
        if snippet and snippet != "(empty response)":
            lines.append(
                "- raw response from the previous attempt "
                "(truncated for feedback):"
            )
            lines.append("  ```")
            for line in snippet.split("\n"):
                lines.append(f"  {line}")
            lines.append("  ```")
        return DiagnosisSection(
            heading="Synthesis errors", severity=SEVERITY_ERROR,
            lines=tuple(lines),
        )

    if source == "empty":
        lines.append(
            "- the previous response came back empty (most likely "
            "truncated by the response budget). Keep comments terse "
            "- at most one intent line per non-trivial function - and "
            "skip datasheet block quotes; the driver source should be "
            "functional, not exhaustively documented."
        )
        return DiagnosisSection(
            heading="Synthesis errors", severity=SEVERITY_ERROR,
            lines=tuple(lines),
        )

    if source == "schema":
        lines.append(
            "- the previous response parsed as JSON but violated "
            "the synthesis schema. Fix each item below before regenerating:"
        )
        shown = 0
        for e in errs:
            if shown >= MAX_SCHEMA_ERRORS_SHOWN:
                break
            lines.append(f"  - {e}")
            shown += 1
        hidden = max(0, len(errs) - shown)
        if hidden:
            lines.append(f"  - ... and {hidden} more schema violation(s)")
        return DiagnosisSection(
            heading="Synthesis errors", severity=SEVERITY_ERROR,
            lines=tuple(lines),
        )

    lines.append(f"- {err}")
    return DiagnosisSection(
        heading="Synthesis errors", severity=SEVERITY_ERROR,
        lines=tuple(lines),
    )


# Stimulus consistency

def render_consistency_report(
    report: Optional[ConsistencyReport],
) -> DiagnosisSection:
    """Turn a :class:`ConsistencyReport` into feedback."""
    if report is None:
        return DiagnosisSection(
            heading="Stimulus self-check", severity=SEVERITY_INFO, lines=(),
        )
    inconsistent = [v for v in report.stimuli if v.verdict == VERDICT_INCONSISTENT]
    llm_only = [v for v in report.stimuli if v.verdict == VERDICT_LLM_ONLY]

    lines: List[str] = []
    severity = SEVERITY_INFO

    if inconsistent:
        severity = SEVERITY_ERROR
        lines.append(
            "- align each stimulus so `expected_read_raw` matches a "
            "plausible byte-order interpretation of its `mock_preload`. "
            "The following entries currently disagree:"
        )
        shown = 0
        for v in inconsistent:
            if shown >= MAX_CONSISTENCY_ISSUES_SHOWN:
                break
            lines.append(f"  - `{v.name}`: {v.evidence}")
            shown += 1
        hidden = max(0, len(inconsistent) - shown)
        if hidden:
            lines.append(f"  - ... and {hidden} more inconsistent stim(s)")
        lines.append(
            "- to fix: either rewrite the `mock_preload` bytes to encode "
            "the desired raw value, or update `expected_read_raw` and "
            "the `derivation` text to match the bytes already declared. "
            "Do not lower `expected_*` to match a driver that returned a "
            "base-unit value when the public adapter/API unit is scaled "
            "(for example scaled engineering units); in that case fix the driver "
            "or adapter unit conversion."
        )

    if llm_only:
        if inconsistent:
            # Keep severity at ERROR while still showing unchecked stimuli.
            pass
        complex_only = [
            v for v in llm_only
            if "complex derivation" in (v.evidence or "")
        ]
        other = [v for v in llm_only if v not in complex_only]
        if complex_only:
            lines.append(
                f"- {len(complex_only)} stimulus/stimuli rely on "
                "multi-coefficient compensation; "
                "the consistency check skips them and they verify only via "
                "Renode probing. Make the `derivation` text spell "
                "out the exact integer or fixed-point algorithm the driver "
                "will execute, so the Renode trace can reproduce the "
                "expected outcome."
            )
        if other:
            lines.append(
                f"- {len(other)} stimulus/stimuli were not statically "
                "checkable before Renode:"
            )
            for v in other[:MAX_CONSISTENCY_ISSUES_SHOWN]:
                lines.append(f"  - `{v.name}`: {v.evidence}")

    return DiagnosisSection(
        heading="Stimulus self-check", severity=severity, lines=tuple(lines),
    )


# Transaction coverage

def _fmt_prefix_set(prefixes: Sequence[Sequence[Any]]) -> str:
    """Format a prefix set like ``"[(0x01), (0xA0, 0x00)]"`` for feedback."""
    if not prefixes:
        return "read-any"
    parts = []
    for pfx in prefixes:
        inner = ", ".join(str(x) for x in pfx)
        parts.append(f"({inner})")
    return "[" + ", ".join(parts) + "]"


def _fmt_addr(addr: Any) -> str:
    """Format an addr_or_pin value - fall back to repr for non-int."""
    if addr is None:
        return "any"
    if isinstance(addr, int):
        return f"0x{addr:02X}"
    if isinstance(addr, str):
        s = addr.strip()
        return s or "any"
    return str(addr)


def _extract_llm_extra_fields(entry: Mapping[str, Any]) -> Tuple[str, str, Sequence[Sequence[Any]]]:
    """Normalize coverage-extra fields for rendering."""
    phase = str(entry.get("phase", "?"))
    addr = entry.get("addr_or_pin", None)
    raw_pfx = entry.get("write_prefix_any_of", ())
    # Normalize to a list-of-lists shape even when the source used a scalar.
    if raw_pfx is None:
        norm_pfx: Tuple[Tuple[Any, ...], ...] = ()
    elif isinstance(raw_pfx, (list, tuple)):
        norm_pfx = tuple(
            tuple(p) if isinstance(p, (list, tuple)) else (p,)
            for p in raw_pfx
        )
    else:
        norm_pfx = ((raw_pfx,),)
    return phase, _fmt_addr(addr), norm_pfx


def render_coverage_report(
    report: Optional[CoverageReport],
) -> DiagnosisSection:
    """Turn a :class:`CoverageReport` into feedback."""
    if report is None:
        return DiagnosisSection(
            heading="Transaction coverage", severity=SEVERITY_INFO, lines=(),
        )
    missing = list(report.missing)
    extras = list(report.llm_extras)
    if not missing and not extras:
        return DiagnosisSection(
            heading="Transaction coverage", severity=SEVERITY_INFO, lines=(),
        )

    lines: List[str] = []
    if missing:
        lines.append(
            "- declare these mechanically-derived required writes in "
            "`expected_transactions` so transaction coverage stops flagging them. "
            "If the driver intentionally skips a row, omit it AND make "
            "sure runtime probing still passes; runtime probing is the final arbiter:"
        )
        shown = 0
        for m in missing:
            if shown >= MAX_COVERAGE_MISSING_SHOWN:
                break
            et = m.mechanical
            lines.append(
                f"  - phase=`{et.phase}` "
                f"addr={_fmt_addr(et.addr_or_pin)} "
                f"prefix_any_of={_fmt_prefix_set(et.write_prefix_any_of)}"
                f" (note: {et.note or 'n/a'})"
            )
            shown += 1
        hidden = max(0, len(missing) - shown)
        if hidden:
            lines.append(f"  - ... and {hidden} more missing transaction(s)")

    if extras:
        lines.append(
            "- the following `expected_transactions` rows have no "
            "mechanical counterpart in `device_ir`. Keep a row only if "
            "the driver really emits that traffic; drop the speculative "
            "ones to tighten the contract:"
        )
        shown = 0
        for e in extras:
            if shown >= MAX_COVERAGE_EXTRAS_SHOWN:
                break
            phase, addr, prefixes = _extract_llm_extra_fields(e)
            lines.append(
                f"  - phase=`{phase}` "
                f"addr={addr} "
                f"prefix_any_of={_fmt_prefix_set(prefixes)}"
            )
            shown += 1
        hidden = max(0, len(extras) - shown)
        if hidden:
            lines.append(f"  - ... and {hidden} more extra transaction(s)")

    return DiagnosisSection(
        heading="Transaction coverage", severity=SEVERITY_WARNING,
        lines=tuple(lines),
    )


# Runtime probe

def _to_float(v: Any) -> Optional[float]:
    if v is None or isinstance(v, bool):
        return None
    if isinstance(v, (int, float)):
        return float(v)
    if isinstance(v, str):
        try:
            return float(v.strip())
        except ValueError:
            return None
    return None


def _raw_scale_hint(outcome: ProbeOutcome, stim: Optional[ProbeStimulus]) -> str:
    """Return a concise generic hint when raw values differ by a unit scale."""
    if stim is None or stim.expected_read_raw is None:
        return ""
    expected = _to_float(stim.expected_read_raw)
    observed = _to_float(outcome.read_raw)
    if expected is None or observed is None or expected == 0 or observed == 0:
        return ""

    ratio = abs(expected / observed)
    inverse = abs(observed / expected)
    scale_candidates = (10.0, 100.0, 1000.0, 1000000.0)
    for scale in scale_candidates:
        if scale * 0.8 <= ratio <= scale * 1.25:
            return (
                f" Numeric scale diagnostic: expected_read_raw/read_raw ~= "
                f"{ratio:.3g} ({expected:g} vs {observed:g}), close to "
                f"{scale:g}x. Treat this as a public-unit conversion bug "
                "unless the derivation proves otherwise: keep `expected_*` "
                "in the adapter/API output unit and change the driver or "
                "adapter calculation so the emitted value has that unit "
                "(for milli/micro units, do not divide away the scale). "
                "If the device LSB is already expressed in milli/micro "
                "units per count, the public output should usually multiply "
                "counts by that LSB directly, not divide it back to base "
                "units."
            )
        if scale * 0.8 <= inverse <= scale * 1.25:
            return (
                f" Numeric scale diagnostic: read_raw/expected_read_raw ~= "
                f"{inverse:.3g} ({observed:g} vs {expected:g}), close to "
                f"{scale:g}x. Treat this as a public-unit conversion bug "
                "unless the derivation proves otherwise: keep `expected_*` "
                "in the adapter/API output unit and change the driver or "
                "adapter calculation so the emitted value has that unit."
            )
    return ""


def _probe_expectation_hint(
    outcome: ProbeOutcome,
    stim: Optional[ProbeStimulus],
    *,
    include_all: bool = False,
) -> str:
    """Return compact expectation details for runtime failures."""
    if stim is None:
        return ""
    check = check_probe_expectations(outcome, stim)
    if check.ok:
        return ""
    failures = check.failures
    if not include_all:
        failures = tuple(
            failure
            for failure in failures
            if "expected_transactions" in failure
        )
    if not failures:
        return ""
    shown = failures[:3]
    suffix = ""
    if len(failures) > len(shown):
        suffix = f"; (+{len(failures) - len(shown)} more expectation issue(s))"
    return " Probe expectation diagnostic: " + "; ".join(shown) + suffix + "."


def _classify_probe_outcome(
    outcome: ProbeOutcome, stim: Optional[ProbeStimulus],
) -> Tuple[str, str]:
    """Categorise one outcome into ``(category, human_reason)``."""
    if outcome.error:
        return "probe_error", f"pipeline error: {outcome.error}"
    if not outcome.boot_detected:
        return (
            "boot_failed",
            "Renode booted the driver but `BOOT_OK` never printed. "
            "To recover: (a) make the adapter symbol set match "
            "`drivergen_eval_adapter.h` exactly, (b) restrict driver "
            "`init()` calls to declared public APIs, (c) ensure "
            "every required TU is in the link set; link-mode compile "
            "may silently drop a TU when symbols are unresolved.",
        )
    if not outcome.test_done:
        return (
            "test_hung",
            "Renode saw `BOOT_OK` but `TEST_DONE` never followed. "
            "To recover: (a) replace any blocking poll on a "
            "slave-emulated ready/valid bit with a bounded retry, "
            "(b) prefer polling over IRQ; the runtime probe does not raise "
            "interrupts, (c) treat any unexpected NACK as terminal; "
            "the runtime probe always ACKs, so an infinite-retry loop on NACK "
            "indicates a logic bug.",
        )
    if (
        stim is not None
        and stim.expected_err is not None
        and int(stim.expected_err) != 0
    ):
        check = check_probe_expectations(outcome, stim)
        if check.ok:
            return "probe_pass", ""
        return (
            "expectation_mismatch",
            "fault/status stimulus expected a nonzero driver error but "
            f"runtime output did not match: {check.summary}. Implement "
            "the Device IR runtime-detectable fault/status handling and "
            "return nonzero from the public read call for this condition.",
        )

    if not outcome.result_pass:
        trace_hint = _probe_expectation_hint(outcome, stim)
        preload = stim.mock_preload if isinstance(stim, ProbeStimulus) else {}
        is_gpio_timing_probe = (
            str(getattr(outcome, "bus_kind", "") or "").lower() == "gpio"
            or (
                isinstance(preload, Mapping)
                and "schedule" in preload
            )
        )
        if (
            not is_gpio_timing_probe
            and (outcome.result_err or outcome.read_err not in (None, 0))
        ):
            expected_hint = ""
            if stim is not None and stim.expected_read_raw is not None:
                expected_hint = f" expected_read_raw={stim.expected_read_raw!r};"
            elif stim is not None and stim.expected_channels:
                expected_hint = " expected_channels declared;"
            elif stim is not None and stim.expected_mem_bytes:
                expected_hint = " expected_mem_bytes declared;"
            return (
                "driver_error",
                f"driver returned an error for a success stimulus:"
                f"{expected_hint} read_err={outcome.read_err}; "
                "fix the runtime I/O setup or transfer sequence before "
                f"debugging value conversion.{trace_hint}",
            )
        # Choose the most specific failure explanation available.
        if outcome.read_channels or (stim is not None and stim.expected_channels):
            bad_channels = []
            if stim is not None and stim.expected_channels:
                tolerance = float(stim.raw_tolerance or 0)
                for ch, exp_raw in stim.expected_channels.items():
                    try:
                        exp = float(exp_raw)
                    except (TypeError, ValueError):
                        continue
                    got = outcome.read_channels.get(ch)
                    if got is None:
                        source_hint = ""
                        preload = getattr(stim, "channel_preload_bytes", None) or {}
                        if isinstance(preload, Mapping) and ch in preload:
                            source_hint = f" source_bytes={preload[ch]!r}"
                        bad_channels.append(
                            f"{ch}: missing in readback{source_hint}"
                        )
                        continue
                    if abs(got - exp) > tolerance:
                        source_hint = ""
                        preload = getattr(stim, "channel_preload_bytes", None) or {}
                        if isinstance(preload, Mapping) and ch in preload:
                            source_hint = f" source_bytes={preload[ch]!r}"
                        bad_channels.append(
                            f"{ch}: expected={exp:g} got={got:g} "
                            f"(tol={tolerance:g}){source_hint}"
                        )
            if bad_channels:
                reason = ("multi_channel mismatch: "
                          + "; ".join(bad_channels[:4]))
                if len(bad_channels) > 4:
                    reason += f" (+{len(bad_channels) - 4} more)"
                return "channel_miss", reason
            if outcome.read_channels:
                # Show populated channels even when none matched.
                got_list = ", ".join(
                    f"{k}={v:g}" for k, v in outcome.read_channels.items()
                )
                return (
                    "channel_miss",
                    f"multi_channel mismatch: readback=[{got_list}] "
                    f"did not satisfy any expected channel",
                )
        if outcome.mem_bytes or (stim and stim.expected_mem_bytes):
            exp_hex = (stim.expected_mem_bytes or "") if stim else ""
            got_hex = "".join(f"{b & 0xFF:02X}" for b in outcome.mem_bytes)
            return (
                "memory_mismatch",
                f"memory readback mismatch: expected='{exp_hex}' "
                f"got='{got_hex}'",
            )
        if outcome.rtc_get_err or outcome.rtc_set_err or outcome.rtc_time:
            parts = []
            if outcome.rtc_set_err:
                parts.append(f"set_err={outcome.rtc_set_err}")
            if outcome.rtc_get_err:
                parts.append(f"get_err={outcome.rtc_get_err}")
            if stim and stim.expected_time and outcome.rtc_time:
                bad = []
                for k, v in stim.expected_time.items():
                    try:
                        exp_i = int(v)
                    except (TypeError, ValueError):
                        continue
                    got = outcome.rtc_time.get(k)
                    if got is not None and int(got) != exp_i:
                        bad.append(f"{k}={got} (expected {exp_i})")
                if bad:
                    parts.append("fields: " + ", ".join(bad[:4]))
            return "rtc_err", "RTC failure: " + ("; ".join(parts) or
                                                 "unknown sub-cause")
        if outcome.display_frame_err or outcome.display_status_err:
            parts = []
            if outcome.display_frame_err:
                parts.append(f"frame_err={outcome.display_frame_err}")
            if outcome.display_status_err:
                parts.append(f"status_err={outcome.display_status_err}")
            return "display_err", "display failure: " + "; ".join(parts)
        if outcome.read_raw is None:
            return (
                "value_unset",
                "adapter returned without writing `*raw` "
                "(`read_raw` stayed `None`). Make every successful "
                "read path store the result into the out-arg before "
                f"returning.{trace_hint}",
            )
        # Value mismatch
        if stim is not None and stim.expected_read_raw is not None:
            exp = stim.expected_read_raw
            got = outcome.read_raw
            tol = stim.raw_tolerance or 0
            preload = stim.mock_preload if isinstance(stim.mock_preload, Mapping) else {}
            if (
                str(getattr(outcome, "bus_kind", "") or "").lower() == "gpio"
                or "schedule" in preload
            ):
                schedule_hint = ""
                schedule = preload.get("schedule") if isinstance(preload, Mapping) else None
                if isinstance(schedule, Sequence) and not isinstance(schedule, (str, bytes)) and schedule:
                    schedule_hint = f" The test drives GPIO schedule={schedule!r}."
                err_hint = (
                    f" Driver returned read_err={outcome.read_err};"
                    if outcome.read_err not in (None, 0) else ""
                )
                return (
                    "value_mismatch",
                    f"GPIO pulse/timing raw value mismatch: expected={exp:g} "
                    f"got={got:g} (tol={tol:g}).{err_hint}{schedule_hint} "
                    "Likely fixes: use the declared fixed-attachment pins "
                    "without guessing or collapsing signals, drive the trigger "
                    "pulse, poll the echo/input pin, and measure high-pulse "
                    "duration with a microsecond delay/timer API. Do not apply "
                    "byte-order/register-address fixes to a GPIO schedule "
                    "stimulus."
                )
            return (
                "value_mismatch",
                f"raw value mismatch: expected={exp:g} got={got:g} "
                f"(tol={tol:g}). Likely fixes: re-check byte-order, "
                f"sign-extension width, and the init opcode set; "
                f"confirm the read register address matches "
                f"`device_ir.registers_or_commands` and that the "
                f"bytes are interpreted per the datasheet.{trace_hint}",
            )
        return (
            "value_mismatch",
            f"result_pass=False with read_raw={outcome.read_raw:g} but "
            f"no expected_read_raw declared; auto-diagnosis is not "
            f"possible; audit the harness trace.",
        )
    check = check_probe_expectations(outcome, stim)
    if not check.ok:
        scale_hint = _raw_scale_hint(outcome, stim)
        unit_hint = (
            " If observed and expected differ mainly by a scale factor "
            "(for example base units vs milli-units), fix the driver/API "
            "unit conversion and keep `expected_*` tied to the public "
            "adapter output unit; do not merely change the test expected "
            "value to match an incorrectly scaled driver result."
        )
        return (
            "expectation_mismatch",
            "firmware printed RESULT: PASS but generated test expectations "
            f"did not match observed output: {check.summary}.{scale_hint}"
            f"{unit_hint}",
        )
    return "probe_pass", ""


def _collect_dropped_preload_warnings(
    stimuli: Optional[Sequence[ProbeStimulus]],
) -> List[str]:
    """Return human-friendly bullet lines for silently dropped preload keys."""
    lines: List[str] = []
    if not stimuli:
        return lines
    any_dropped = False
    for stim in stimuli:
        dropped = getattr(stim, "dropped_preload_keys", ()) or ()
        if not dropped:
            continue
        any_dropped = True
        preview = ", ".join(f"`{k}` ({reason})" for k, reason in dropped[:5])
        remainder = len(dropped) - 5
        more = f" +{remainder} more" if remainder > 0 else ""
        lines.append(
            f"  - `{stim.name}`: silently dropped {len(dropped)} "
            f"mock_preload key(s) -> {preview}{more}"
        )
    if not any_dropped:
        return lines
    lines.insert(
        0,
        "- Some stimulus `mock_preload` entries used keys the slave "
        "renderer cannot accept; the Renode slave then served `0xFF` "
        "for those registers and the driver's init-time chip-ID / "
        "status-register check failed. Rewrite each key into one of "
        "these accepted shapes: hex register literal (`\"0xD0\"`), "
        "prefixed register (`\"reg_0xD0\"`, `\"req_0xD0\"`, "
        "`\"resp_0xD0\"`), addr:reg pair (`\"0x76:0xD0\"`), or a "
        "named sentinel (`\"read_bytes\"`, `\"schedule\"`, "
        "`\"payload\"`, `\"frame_ok\"`, `\"status_err\"`). Drop the "
        "init-time chip-ID check only if the driver genuinely tolerates "
        "`0xFF` from those registers.",
    )
    return lines


def render_probe_outcomes(
    outcomes: Optional[Sequence[ProbeOutcome]],
    stimuli: Optional[Sequence[ProbeStimulus]] = None,
) -> DiagnosisSection:
    """Turn a list of :class:`ProbeOutcome` into feedback."""
    if outcomes is None:
        return DiagnosisSection(
            heading="Runtime probe (Renode)", severity=SEVERITY_INFO,
            lines=(),
        )
    if not outcomes:
        return DiagnosisSection(
            heading="Runtime probe (Renode)", severity=SEVERITY_INFO,
            lines=(),
        )
    stim_map = {s.name: s for s in (stimuli or [])}
    failures: List[Tuple[ProbeOutcome, str, str]] = []
    for o in outcomes:
        cat, reason = _classify_probe_outcome(o, stim_map.get(o.stimulus_name))
        if cat != "probe_pass":
            failures.append((o, cat, reason))

    dropped_bullets = _collect_dropped_preload_warnings(stimuli)

    if not failures:
        # Keep a traceable pass note when other sections still need repair.
        info_lines: List[str] = [
            f"- all {len(outcomes)} stimuli passed on Renode "
            "(the driver's I/O behaviour is correct for the stims "
            "you declared).",
        ]
        if dropped_bullets:
            info_lines.extend(dropped_bullets)
        return DiagnosisSection(
            heading="Runtime probe (Renode)",
            severity=(SEVERITY_WARNING if dropped_bullets else SEVERITY_INFO),
            lines=tuple(info_lines),
        )

    lines: List[str] = [
        f"- {len(failures)} of {len(outcomes)} stimulus vector(s) "
        "failed in Renode:",
    ]
    shown = 0
    for o, cat, reason in failures:
        if shown >= MAX_PROBE_FAILURES_SHOWN:
            break
        lines.append(f"  - `{o.stimulus_name}` [{cat}]: {reason}")
        shown += 1
    hidden = max(0, len(failures) - shown)
    if hidden:
        lines.append(f"  - ... and {hidden} more failing stim(s)")

    if dropped_bullets:
        lines.extend(dropped_bullets)

    return DiagnosisSection(
        heading="Runtime probe (Renode)", severity=SEVERITY_ERROR,
        lines=tuple(lines),
    )


# Compile diagnostics

def render_compile_errors(
    errors: Optional[Sequence[str]],
) -> DiagnosisSection:
    """Render link-mode compile errors as a feedback section."""
    if not errors:
        return DiagnosisSection(
            heading="Compile errors", severity=SEVERITY_INFO, lines=(),
        )
    lines: List[str] = []

    # Detect missing-header diagnostics across common compiler formats.
    missing_header_re = re.compile(
        r"(?:fatal\s+)?error:\s*[\"<']?([^\"<>'\s:]+\.h)[\"<>']?:\s*No such file or directory",
        re.IGNORECASE,
    )
    missing: List[str] = []
    for e in errors:
        for m in missing_header_re.finditer(e):
            h = m.group(1).strip()
            if h not in missing:
                missing.append(h)
    if missing:
        lines.append(
            "- *MISSING HEADER*: the previous attempt tried to "
            "`#include` a header that the stub-compile sandbox cannot "
            "resolve. The exact missing basename(s) below are now "
            "**forbidden for the next attempt**, even if a stale contract "
            "entry listed them. Drop the include when the symbol is unused; "
            "otherwise switch to a public header that actually compiles in "
            "the stub environment. Do not guess vendor or internal header "
            "names. Forbidden missing headers:"
        )
        for h in missing:
            lines.append(f"    - `{h}`")
        lines.append("")

    suggestion_re = re.compile(
        r"implicit declaration of function\s+['`\"](?P<bad>[A-Za-z_][A-Za-z0-9_]*)['`\"]"
        r".*?did you mean\s+['`\"](?P<good>[A-Za-z_][A-Za-z0-9_]*)['`\"]",
        re.IGNORECASE,
    )
    suggested: List[tuple[str, str]] = []
    for e in errors:
        for m in suggestion_re.finditer(e):
            pair = (m.group("bad"), m.group("good"))
            if pair not in suggested:
                suggested.append(pair)
    if suggested:
        lines.append(
            "- *SYMBOL SURFACE MISMATCH*: the compiler found a likely "
            "public replacement for an undeclared symbol. Treat the "
            "undeclared symbol as forbidden in the next attempt when the "
            "replacement appears in the public surface or a stub header:"
        )
        for bad, good in suggested:
            lines.append(f"    - replace forbidden `{bad}` with public `{good}`")
        lines.append("")

    bad_field_re = re.compile(
        r"error:\s*['`\"]struct\s+(?P<struct>[A-Za-z_][A-Za-z0-9_]*)['`\"]"
        r"\s+has no member named\s+['`\"](?P<field>[A-Za-z_][A-Za-z0-9_]*)['`\"]",
        re.IGNORECASE,
    )
    bad_fields: List[tuple[str, str]] = []
    for e in errors:
        for m in bad_field_re.finditer(e):
            pair = (m.group("struct"), m.group("field"))
            if pair not in bad_fields:
                bad_fields.append(pair)
    if bad_fields:
        lines.append(
            "- *STRUCT FIELD MISMATCH*: the previous attempt used field "
            "names that are not present in the target struct definition. In "
            "the next attempt, use only the struct fields rendered in "
            "the struct field allow-list:"
        )
        for struct_name, field in bad_fields:
            lines.append(
                f"    - `struct {struct_name}` has no field `{field}`"
            )
        lines.append("")

    lines.append(
        "- the previous driver failed to compile (syntax or link). "
        "Fix these arm-gcc errors:"
    )
    for e in list(errors)[:15]:
        e = e.rstrip()
        # indent every continuation line to keep the bullet compact
        for i, ln in enumerate(e.split("\n")):
            prefix = "  - " if i == 0 else "    "
            lines.append(f"{prefix}{ln}")
    if len(errors) > 15:
        lines.append(f"  - ... and {len(errors) - 15} more compile diagnostic(s)")
    return DiagnosisSection(
        heading="Compile errors", severity=SEVERITY_ERROR, lines=tuple(lines),
    )


def render_critic_failures(
    failures: Optional[Sequence[Mapping[str, Any]]],
) -> DiagnosisSection:
    """Render critic verdict failures as a feedback section."""
    if not failures:
        return DiagnosisSection(
            heading="Static critics", severity=SEVERITY_INFO, lines=(),
        )
    lines: List[str] = [
        "- the previous driver failed static critics:",
    ]
    for f in failures:
        cid = f.get("constraint_id", "?")
        ctype = f.get("constraint_type", "?")
        msg = f.get("message", "(no detail)")
        lines.append(f"  - [{cid}] {ctype}: {msg}")
    return DiagnosisSection(
        heading="Static critics", severity=SEVERITY_ERROR, lines=tuple(lines),
    )


# Invariants (top section, from invariants_tracker)

def render_invariants(
    invariant_lines: Optional[Sequence[str]],
) -> DiagnosisSection:
    """Render confirmed invariants as the FIRST feedback section."""
    if not invariant_lines:
        return DiagnosisSection(
            heading="Confirmed Invariants (sticky across rounds)",
            severity=SEVERITY_INFO, lines=(),
        )
    lines = tuple(
        ln if ln.startswith("-") or ln.startswith("  ") else f"- {ln}"
        for ln in invariant_lines
    )
    return DiagnosisSection(
        heading="Confirmed Invariants (sticky across rounds)",
        severity=SEVERITY_INFO, lines=lines,
    )


# Task closer (always-on final section)

def _task_section(has_errors: bool, has_warnings: bool) -> DiagnosisSection:
    """Append the standard task directive."""
    if not has_errors and not has_warnings:
        return DiagnosisSection(heading="Task", severity=SEVERITY_INFO, lines=())
    if has_errors:
        lines = (
            "- Respect every invariant above.",
            "- Fix every issue marked as an error; they are blocking.",
            "- Address warnings where feasible; justify via runtime evidence "
            "outcomes when you deliberately leave one.",
            "- Regenerate all 4 top-level JSON keys "
            "(`driver_header`, `driver_source`, `api_contract`, "
            "`test_plan`) as a single bare JSON object.",
        )
    else:
        lines = (
            "- Respect every invariant above.",
            "- Address the warnings where feasible.",
            "- Regenerate all 4 top-level JSON keys as a single bare "
            "JSON object.",
        )
    return DiagnosisSection(heading="Task", severity=SEVERITY_INFO, lines=lines)


# Top-level composer

def compose_diagnosis(
    attempt: int,
    *,
    invariant_lines: Optional[Sequence[str]] = None,
    synthesis_error: Optional[SynthesisError] = None,
    consistency: Optional[ConsistencyReport] = None,
    coverage: Optional[CoverageReport] = None,
    probe_outcomes: Optional[Sequence[ProbeOutcome]] = None,
    probe_stimuli: Optional[Sequence[ProbeStimulus]] = None,
    compile_errors: Optional[Sequence[str]] = None,
    critic_failures: Optional[Sequence[Mapping[str, Any]]] = None,
) -> DiagnosisReport:
    """Assemble every section into a :class:`DiagnosisReport`."""
    inv_sec = render_invariants(invariant_lines)
    syn_sec = render_synthesis_error(synthesis_error)
    compile_sec = render_compile_errors(compile_errors)
    critic_sec = render_critic_failures(critic_failures)
    cons_sec = render_consistency_report(consistency)
    cov_sec = render_coverage_report(coverage)
    probe_sec = render_probe_outcomes(probe_outcomes, probe_stimuli)

    non_task_sections = (
        inv_sec, syn_sec, compile_sec, critic_sec,
        cons_sec, cov_sec, probe_sec,
    )
    has_errors = any(s.severity == SEVERITY_ERROR and s.lines
                     for s in non_task_sections)
    has_warnings = any(s.severity == SEVERITY_WARNING and s.lines
                       for s in non_task_sections)

    task_sec = _task_section(has_errors, has_warnings)
    return DiagnosisReport(
        attempt=attempt,
        sections=non_task_sections + (task_sec,),
    )


def render_feedback(
    attempt: int,
    *,
    invariant_lines: Optional[Sequence[str]] = None,
    synthesis_error: Optional[SynthesisError] = None,
    consistency: Optional[ConsistencyReport] = None,
    coverage: Optional[CoverageReport] = None,
    probe_outcomes: Optional[Sequence[ProbeOutcome]] = None,
    probe_stimuli: Optional[Sequence[ProbeStimulus]] = None,
    compile_errors: Optional[Sequence[str]] = None,
    critic_failures: Optional[Sequence[Mapping[str, Any]]] = None,
) -> str:
    """Convenience wrapper: compose + render in one call."""
    return compose_diagnosis(
        attempt,
        invariant_lines=invariant_lines,
        synthesis_error=synthesis_error,
        consistency=consistency,
        coverage=coverage,
        probe_outcomes=probe_outcomes,
        probe_stimuli=probe_stimuli,
        compile_errors=compile_errors,
        critic_failures=critic_failures,
    ).render()


__all__ = [
    "SEVERITY_ERROR",
    "SEVERITY_WARNING",
    "SEVERITY_INFO",
    "DiagnosisSection",
    "DiagnosisReport",
    "render_synthesis_error",
    "render_consistency_report",
    "render_coverage_report",
    "render_probe_outcomes",
    "render_compile_errors",
    "render_critic_failures",
    "render_invariants",
    "compose_diagnosis",
    "render_feedback",
]
