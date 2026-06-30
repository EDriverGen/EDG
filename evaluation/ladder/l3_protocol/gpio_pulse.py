"""evaluation.ladder.l3_protocol.gpio_pulse - GPIO pulse-protocol judge."""
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Literal, Optional, Tuple

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import OracleData
from evaluation.runtime.gpio_pulse_runner import GpioVectorOutcome

Policy = Literal["auto", "strict", "semantic", "relaxed"]

# Canonical 1-wire event sequence (DHT22, DS18B20 reset, etc).
_CANON = ("mcu_low", "mcu_release", "playback_done")
# Extended set recognised by the semantic checker (`mcu_release_auto` is
# the dual-pin equivalent emitted by the injector for HCSR04 etc).
_CANON_KNOWN = _CANON + ("mcu_release_auto",)


# ---------- trace primitives ----------

@dataclass(frozen=True)
class GpioEvent:
    event: str
    total_us: Optional[int] = None


def _load_gpio_trace(path: Path) -> List[GpioEvent]:
    """Parse a JSONL GPIO trace into a list of GpioEvent."""
    out: List[GpioEvent] = []
    if not path.exists():
        return out
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            continue
        ev = str(obj.get("event", ""))
        if not ev:
            continue
        total = obj.get("total_us")
        try:
            total_int: Optional[int] = int(total) if total is not None else None
        except (TypeError, ValueError):
            total_int = None
        out.append(GpioEvent(event=ev, total_us=total_int))
    return out


# ---------- policy resolution ----------

_DALLAS_BITSLOT_HINTS = {
    "1-wire-bitslot",
    "bitslot-1wire",
    "dallas_1wire_bitslot",
    "dallas-1wire-bitslot",
    "dallas_1wire",
    "dallas-1wire",
}


def _is_dallas_bitslot_oracle(oracle: OracleData) -> bool:
    hint = str(getattr(oracle.meta, "gpio_protocol_hint", "") or "").lower()
    return hint in _DALLAS_BITSLOT_HINTS


def _resolve_policy(oracle: OracleData, requested: Policy) -> Tuple[str, str]:
    if requested != "auto":
        return requested, f"policy={requested!r} (explicit)"
    if _is_dallas_bitslot_oracle(oracle):
        return (
            "relaxed",
            "auto-selected relaxed for Dallas 1-Wire bit-slot "
            "(partial scratchpad reads are legal)",
        )
    # GPIO oracles rarely carry a golden_trace; default to semantic,
    # which encodes the canonical 1-wire handshake.
    return "semantic", "auto-selected semantic (canonical handshake check)"


def _claim_for(policy: str) -> str:
    return {
        "strict":   "protocol-valid-strict",
        "semantic": "protocol-valid-semantic",
        "relaxed":  "protocol-valid-relaxed",
    }[policy]


# ---------- comparators ----------

def _cmp_strict(events: List[GpioEvent]) -> Tuple[bool, str, Dict[str, Any]]:
    names = tuple(e.event for e in events)
    if names == _CANON:
        return True, "canonical one-shot: mcu_low → mcu_release → playback_done", {
            "events": list(names),
        }
    return False, (
        f"strict expects events exactly == {list(_CANON)}, got {list(names)}"
    ), {"events": list(names)}


def _cmp_semantic(events: List[GpioEvent]) -> Tuple[bool, str, Dict[str, Any]]:
    """mcu_low → mcu_release → playback_done must appear as an ordered subsequence somewhere."""
    pending_low = False
    saw_release_in_pair = False
    saw_done_after_release = False
    pairs = 0
    for ev in events:
        if ev.event == "mcu_low":
            pending_low = True
            saw_release_in_pair = False
        elif ev.event in ("mcu_release", "mcu_release_auto"):
            if not pending_low:
                return False, (
                    f"{ev.event} without a preceding mcu_low "
                    f"(events={[e.event for e in events]})"
                ), {"events": [e.event for e in events]}
            saw_release_in_pair = True
        elif ev.event == "playback_done":
            if saw_release_in_pair:
                saw_done_after_release = True
                pairs += 1
                pending_low = False
                saw_release_in_pair = False
    if pairs >= 1:
        return True, (
            f"observed {pairs} full handshake(s) in "
            f"{len([e for e in events if e.event in _CANON_KNOWN])} canonical events"
        ), {"pairs": pairs, "events": [e.event for e in events]}
    if saw_release_in_pair and not saw_done_after_release:
        return False, (
            "handshake started but playback_done never observed — "
            "driver likely gave up before sensor finished"
        ), {"events": [e.event for e in events]}
    return False, (
        f"no canonical handshake in events: {[e.event for e in events]}"
    ), {"events": [e.event for e in events]}


def _cmp_relaxed(events: List[GpioEvent]) -> Tuple[bool, str, Dict[str, Any]]:
    """Both `mcu_low` and `mcu_release` appear at least once in any
    order — minimum we can demand of a driver that may have timed
    out before the sensor replied."""
    names = {e.event for e in events}
    if "mcu_low" in names and "mcu_release" in names:
        return True, (
            f"handshake edges observed: {sorted(names)}"
        ), {"events": [e.event for e in events]}
    return False, (
        f"missing handshake edges (mcu_low, mcu_release) in {sorted(names)}"
    ), {"events": [e.event for e in events]}


# ---------- main judge ----------

def judge(
    device_id: str,
    vector_outcomes: Iterable[GpioVectorOutcome],
    oracle: OracleData,
    *,
    policy: Policy = "auto",
) -> LevelVerdict:
    """Aggregate per-vector L3 verdicts into a single overall L3 verdict."""
    outcomes = list(vector_outcomes)
    resolved, reason = _resolve_policy(oracle, policy)
    claim = _claim_for(resolved)

    if not outcomes:
        return LevelVerdict(
            device=device_id, level="L3", passed=False,
            claim=claim,
            detail="no vectors to evaluate",
            evidence={"policy": resolved, "requested_policy": policy},
        )

    per_vector: List[dict] = []
    any_fail = False
    for out in outcomes:
        sub = _judge_one_vector(out, resolved)
        per_vector.append(sub)
        if not sub["passed"]:
            any_fail = True

    passed = not any_fail
    pass_count = sum(1 for p in per_vector if p["passed"])
    total = len(per_vector)
    if passed:
        detail = f"{pass_count}/{total} vectors match oracle under {resolved} policy"
    else:
        worst = next(p for p in per_vector if not p["passed"])
        detail = (
            f"{pass_count}/{total} vectors passed {resolved}; "
            f"first failure: {worst['stimulus']} — {worst['detail']}"
        )

    return LevelVerdict(
        device=device_id, level="L3", passed=passed,
        claim=claim,
        detail=f"{reason}; {detail}" if policy == "auto" else detail,
        evidence={
            "policy":           resolved,
            "requested_policy": policy,
            "total":            total,
            "passed":           pass_count,
            "failed_count":     total - pass_count,
            "per_vector":       per_vector[:16],
            "bus_type":         oracle.meta.bus_type,
        },
    )


def _judge_one_vector(
    outcome: GpioVectorOutcome,
    resolved_policy: str,
) -> dict:
    stim = outcome.stimulus_name
    if outcome.trace_path is None or not Path(outcome.trace_path).exists():
        return {"stimulus": stim, "passed": False,
                "detail": f"no trace file captured (path={outcome.trace_path})"}
    try:
        events = _load_gpio_trace(Path(outcome.trace_path))
    except Exception as e:
        return {"stimulus": stim, "passed": False,
                "detail": f"trace load error: {e!r}"}

    if resolved_policy == "strict":
        ok, detail, ev = _cmp_strict(events)
    elif resolved_policy == "semantic":
        ok, detail, ev = _cmp_semantic(events)
    elif resolved_policy == "relaxed":
        ok, detail, ev = _cmp_relaxed(events)
    else:
        return {"stimulus": stim, "passed": False,
                "detail": f"unknown policy {resolved_policy!r}"}

    ret = {
        "stimulus": stim,
        "passed":   bool(ok),
        "detail":   detail,
        "event_count": len(events),
    }
    ret.update(ev)
    return ret


__all__ = ["judge", "Policy", "GpioEvent"]
