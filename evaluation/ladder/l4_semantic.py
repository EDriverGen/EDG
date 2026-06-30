"""evaluation.ladder.l4_semantic - L4 semantic-valid judge."""
from __future__ import annotations

from typing import Any, Dict, Iterable, List, Optional, Tuple

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import ExpectedReading, OracleData, PhysicalRange, Stimulus
from evaluation.runtime.i2c_runner import VectorOutcome


def _match_reading(
    reading: Optional[float],
    expected: Optional[int],
    raw_alt: List[int],
    tolerance: int,
) -> Tuple[bool, Optional[int]]:
    """Return (passed, matched_value). If no explicit value is defined
    here (no expected and no alts), returns (False, None) so the caller
    can fall back to the physical_range check."""
    if reading is None:
        return False, None
    accept: List[int] = []
    if expected is not None:
        accept.append(int(expected))
    accept.extend(int(x) for x in raw_alt)
    if not accept:
        return False, None
    for target in accept:
        if abs(reading - target) <= tolerance:
            return True, target
    return False, None


def _channel_spec_lists(
    spec: Dict[str, Any], default_tol: int,
) -> Tuple[Optional[int], List[int], int]:
    """Extract raw, raw_alt list, tolerance from one channel's oracle object."""
    raw_v = spec.get("raw")
    raw_int = int(raw_v) if raw_v is not None else None
    ra = spec.get("raw_alt", [])
    ra_list = [int(x) for x in ra] if isinstance(ra, list) else []
    tol = int(spec.get("tolerance", default_tol))
    return raw_int, ra_list, tol


_RTC_FIELDS = ("year", "month", "day", "hour", "minute", "second", "weekday")


def _match_rtc(
    out: VectorOutcome,
    exp: ExpectedReading,
) -> Tuple[bool, dict]:
    """For RTC devices, verify: 1."""
    want_err = exp.err if exp.err is not None else 0
    got_err = out.rtc_get_err
    if got_err is None:
        return False, {
            "stimulus": out.stimulus_name,
            "passed": False,
            "detail": "no rtc_get_err observed (harness output missing)",
            "match_source": "rtc",
            "expected_err": want_err,
        }
    if got_err != want_err:
        return False, {
            "stimulus": out.stimulus_name,
            "passed": False,
            "detail": f"rtc_get_err mismatch: got {got_err}, expected {want_err}",
            "match_source": "rtc",
            "expected_err": want_err,
            "got_err": got_err,
        }

    if exp.time:
        mismatches: List[str] = []
        for k in _RTC_FIELDS:
            if k not in exp.time:
                continue
            want = int(exp.time[k])
            got = out.rtc_time.get(k)
            if got is None:
                mismatches.append(f"missing {k}")
                continue
            if int(got) != want:
                mismatches.append(f"{k}={got} (expected {want})")
        if mismatches:
            return False, {
                "stimulus": out.stimulus_name,
                "passed": False,
                "detail": "rtc time mismatch: " + "; ".join(mismatches),
                "match_source": "rtc",
                "expected": dict(exp.time),
                "got": dict(out.rtc_time),
            }

    return True, {
        "stimulus": out.stimulus_name,
        "passed": True,
        "detail": "rtc_get_err matches + time fields matched"
        if exp.time else "rtc_get_err matches (no time check requested)",
        "match_source": "rtc",
        "expected_err": want_err,
        "got_err": got_err,
        "expected": dict(exp.time) if exp.time else None,
        "got": dict(out.rtc_time) if out.rtc_time else None,
    }


def _match_display(
    out: VectorOutcome,
    exp: ExpectedReading,
) -> Tuple[bool, dict]:
    """For display devices, compare the frame output return code."""
    want = exp.err if exp.err is not None else 0
    got = out.display_frame_err
    if got is None:
        return False, {
            "stimulus": out.stimulus_name,
            "passed": False,
            "detail": "no output_frame_err observed (harness output missing)",
            "match_source": "display",
            "expected_err": want,
            "got_err": None,
        }
    if got != want:
        return False, {
            "stimulus": out.stimulus_name,
            "passed": False,
            "detail": (
                f"output_frame_err mismatch: got {got}, expected {want}"
            ),
            "match_source": "display",
            "expected_err": want,
            "got_err": got,
            "status_err": out.display_status_err,
            "status": out.display_status,
        }
    return True, {
        "stimulus": out.stimulus_name,
        "passed": True,
        "detail": f"display frame err={got} matches expected {want}",
        "match_source": "display",
        "expected_err": want,
        "got_err": got,
        "status_err": out.display_status_err,
        "status": out.display_status,
    }


def _match_memory(
    out: VectorOutcome,
    exp: ExpectedReading,
) -> Tuple[bool, dict]:
    """When ``exp.mem_bytes`` is non-empty, compare byte-by-byte against
    ``out.mem_bytes``. The read must cover at least ``len(exp.mem_bytes)``
    bytes; the prefix must match exactly."""
    got = list(out.mem_bytes or [])
    want = list(exp.mem_bytes)
    if len(got) < len(want):
        return False, {
            "stimulus": out.stimulus_name,
            "passed": False,
            "detail": (
                f"short mem_read: got {len(got)} bytes, "
                f"expected at least {len(want)}"
            ),
            "reading": out.read_raw,
            "match_source": "memory",
            "mem_bytes_got": got,
            "mem_bytes_want": want,
        }
    for i, (e, a) in enumerate(zip(want, got)):
        if e != a:
            return False, {
                "stimulus": out.stimulus_name,
                "passed": False,
                "detail": (
                    f"mem_bytes[{i}] mismatch: got 0x{a:02x}, "
                    f"expected 0x{e:02x}"
                ),
                "reading": out.read_raw,
                "match_source": "memory",
                "mem_bytes_got": got[: max(len(want), 16)],
                "mem_bytes_want": want,
            }
    return True, {
        "stimulus": out.stimulus_name,
        "passed": True,
        "detail": f"mem_bytes matched ({len(want)} bytes)",
        "reading": out.read_raw,
        "match_source": "memory",
        "mem_bytes_got": got[: max(len(want), 16)],
        "mem_bytes_want": want,
    }


def _match_multi_channel(
    out: VectorOutcome,
    exp: ExpectedReading,
    rng: Optional[PhysicalRange],
) -> Tuple[bool, dict]:
    """When ``exp.channels`` is non-empty, match each channel id to
    ``out.read_channels[ch_id]`` (with channel-0 fallback to ``read_raw``)."""
    ch_reads = dict(out.read_channels)
    keys = list(exp.channels.keys())
    per_ch: List[dict] = []
    for i, ch_id in enumerate(keys):
        spec = exp.channels[ch_id]
        if not isinstance(spec, dict):
            return False, {
                "stimulus": out.stimulus_name,
                "passed": False,
                "detail": f"channel {ch_id!r}: spec is not an object",
                "match_source": "multi_channel",
                "channels": per_ch,
            }
        raw_int, ra_list, tol = _channel_spec_lists(spec, exp.tolerance)
        reading: Optional[float] = ch_reads.get(ch_id)
        # Fallback: index-based matching when the adapter uses
        # different channel names than the oracle (e.g. adapter
        # outputs "gpio_a0" but oracle expects "gpa0").
        if reading is None:
            _sorted_read_keys = sorted(ch_reads.keys())
            if i < len(_sorted_read_keys):
                reading = ch_reads.get(_sorted_read_keys[i])
        if reading is None and i == 0 and out.read_raw is not None:
            reading = out.read_raw
        ok, matched = _match_reading(reading, raw_int, ra_list, tol)
        src = "multi_channel"
        if not ok and raw_int is None and not ra_list:
            r_pass, r_detail = _range_check(reading, rng)
            ok = r_pass
            src = "physical_range_fallback" if r_pass else "multi_channel"
        per_ch.append({
            "channel": ch_id, "passed": ok, "reading": reading,
            "expected": raw_int, "raw_alt": ra_list, "tolerance": tol,
            "matched": matched, "match_source": src,
        })
        if not ok:
            return False, {
                "stimulus": out.stimulus_name,
                "passed": False,
                "detail": (
                    f"multi_channel {ch_id}: reading {reading} "
                    f"!= expected {raw_int} (±{tol}) or alt {ra_list}"
                ),
                "reading": out.read_raw,
                "match_source": "multi_channel",
                "channels": per_ch,
            }
    return True, {
        "stimulus": out.stimulus_name,
        "passed": True,
        "detail": "all channels matched",
        "reading": out.read_raw,
        "match_source": "multi_channel",
        "channels": per_ch,
    }


def _range_check(
    reading: Optional[float], rng: Optional[PhysicalRange]
) -> Tuple[bool, str]:
    """Range fallback. Passes iff rng exists, reading is not None, and
    reading is in [min, max]."""
    if rng is None:
        return False, "no per-vector expected AND no physical_range fallback"
    if reading is None:
        return False, "no reading captured"
    if rng.minimum <= float(reading) <= rng.maximum:
        return True, f"reading {reading} in physical_range [{rng.minimum}, {rng.maximum}]"
    return False, f"reading {reading} out of physical_range [{rng.minimum}, {rng.maximum}]"


def judge(
    device_id: str,
    vector_outcomes: Iterable[VectorOutcome],
    oracle: OracleData,
) -> LevelVerdict:
    """L4 verdict aggregated across all vectors."""
    outcomes = list(vector_outcomes)
    total = len(outcomes)
    stim_by_name: Dict[str, Stimulus] = {s.name: s for s in oracle.stimuli}
    rng = oracle.physical_range

    if total == 0:
        return LevelVerdict(
            device=device_id, level="L4", passed=False, claim="semantic-valid",
            detail="no vectors to evaluate",
            evidence={"total": 0, "per_vector": []},
        )

    per_vector: List[dict] = []
    any_fail = False
    eval_class = oracle.meta.eval_class
    for out in outcomes:
        stim = stim_by_name.get(out.stimulus_name)
        if stim is None:
            per_vector.append({
                "stimulus": out.stimulus_name,
                "passed": False,
                "detail": "stimulus not found in oracle — name mismatch",
                "reading": out.read_raw,
            })
            any_fail = True
            continue

        exp = stim.expected
        if eval_class == "display":
            ok, pv = _match_display(out, exp)
            per_vector.append(pv)
            if not ok:
                any_fail = True
            continue
        if eval_class == "rtc":
            ok, pv = _match_rtc(out, exp)
            per_vector.append(pv)
            if not ok:
                any_fail = True
            continue
        if exp.mem_bytes:
            ok, pv = _match_memory(out, exp)
            per_vector.append(pv)
            if not ok:
                any_fail = True
            continue
        if exp.channels:
            ok, pv = _match_multi_channel(out, exp, rng)
            per_vector.append(pv)
            if not ok:
                any_fail = True
            continue

        # Expected-error vectors: if oracle says err!=0, check result_err flag
        if exp.err is not None and exp.raw is None and not exp.raw_alt:
            got_err = getattr(out, "result_err", False)
            ok = bool(got_err) if exp.err != 0 else not got_err
            per_vector.append({
                "stimulus":     out.stimulus_name,
                "passed":       ok,
                "detail":       (f"expected_err={exp.err}, got_err={got_err}"
                                 + ("" if ok else " — mismatch")),
                "reading":      out.read_raw,
                "match_source": "expected_err",
            })
            if not ok:
                any_fail = True
            continue

        passed, matched = _match_reading(
            out.read_raw, exp.raw, exp.raw_alt, exp.tolerance
        )
        if passed:
            per_vector.append({
                "stimulus":     out.stimulus_name,
                "passed":       True,
                "detail":       f"matched {matched} (±{exp.tolerance})",
                "reading":      out.read_raw,
                "expected":     exp.raw,
                "raw_alt":      exp.raw_alt,
                "tolerance":    exp.tolerance,
                "match_source": "per_vector_raw",
            })
            continue

        # Explicit accept list failed → if both raw and raw_alt empty, use range fallback.
        if exp.raw is None and not exp.raw_alt:
            r_pass, r_detail = _range_check(out.read_raw, rng)
            per_vector.append({
                "stimulus":     out.stimulus_name,
                "passed":       r_pass,
                "detail":       r_detail,
                "reading":      out.read_raw,
                "match_source": "physical_range_fallback",
            })
            if not r_pass:
                any_fail = True
            continue

        # Per-vector expected was defined but didn't match; no range fallback.
        any_fail = True
        per_vector.append({
            "stimulus":     out.stimulus_name,
            "passed":       False,
            "detail":       (f"reading {out.read_raw} != expected "
                             f"{exp.raw} (±{exp.tolerance}) or raw_alt {exp.raw_alt}"),
            "reading":      out.read_raw,
            "expected":     exp.raw,
            "raw_alt":      exp.raw_alt,
            "tolerance":    exp.tolerance,
            "match_source": "per_vector_raw",
        })

    pass_count = sum(1 for p in per_vector if p["passed"])
    passed = not any_fail

    if passed:
        detail = f"{pass_count}/{total} vectors matched per-vector expected raw"
    else:
        worst = next(p for p in per_vector if not p["passed"])
        detail = (
            f"{pass_count}/{total} vectors passed L4; "
            f"first failure: {worst['stimulus']} — {worst['detail']}"
        )

    return LevelVerdict(
        device=device_id, level="L4", passed=passed, claim="semantic-valid",
        detail=detail,
        evidence={
            "total":         total,
            "passed":        pass_count,
            "failed_count":  total - pass_count,
            "per_vector":    per_vector[:16],
        },
    )


__all__ = ["judge"]
