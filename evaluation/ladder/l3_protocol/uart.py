"""evaluation.ladder.l3_protocol.uart - UART protocol-valid judge."""
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Literal, Optional, Tuple

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import OracleData, RequiredWrite
from evaluation.runtime.uart_runner import UartVectorOutcome

Policy = Literal["auto", "strict", "semantic", "relaxed"]


# ---------- trace primitives ----------

@dataclass(frozen=True)
class UartFrame:
    """One matched request / response pair on USART1."""
    req: List[int]
    resp: List[int]


def _hex_to_bytes(s: str) -> List[int]:
    """Parse concatenated uppercase hex (e.g."""
    s = s.strip()
    if s.startswith(("0x", "0X")):
        s = s[2:]
    s = s.replace(" ", "").replace("_", "")
    if len(s) % 2:
        s = s[:-1]
    out: List[int] = []
    for i in range(0, len(s), 2):
        try:
            out.append(int(s[i:i + 2], 16))
        except ValueError:
            return out  # stop at first invalid pair
    return out


def _load_uart_trace(path: Path) -> List[UartFrame]:
    """Parse a JSONL UART trace into a list of UartFrame."""
    out: List[UartFrame] = []
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
        try:
            out.append(UartFrame(
                req=_hex_to_bytes(str(obj.get("req_hex", ""))),
                resp=_hex_to_bytes(str(obj.get("resp_hex", ""))),
            ))
        except (KeyError, ValueError, TypeError):
            continue
    return out


# ---------- policy resolution ----------

def _resolve_policy(oracle: OracleData, requested: Policy) -> Tuple[str, str]:
    if requested != "auto":
        return requested, f"policy={requested!r} (explicit)"
    if oracle.golden_trace is not None:
        return "semantic", "auto-selected semantic (golden_trace present)"
    if oracle.required_writes:
        return "relaxed", "auto-selected relaxed (required_writes present)"
    return "vacuous", "no oracle artifacts — L3 vacuously satisfied"


def _claim_for(policy: str) -> str:
    return {
        "strict":   "protocol-valid-strict",
        "semantic": "protocol-valid-semantic",
        "relaxed":  "protocol-valid-relaxed",
        "vacuous":  "protocol-valid-relaxed",
    }[policy]


def _golden_frames(raw: Optional[Dict[str, Any]]) -> List[UartFrame]:
    """Convert oracle.golden_trace into a list of UartFrame."""
    if not raw:
        return []
    frames_raw = raw.get("frames") or raw.get("transactions", [])
    out: List[UartFrame] = []
    for f in frames_raw:
        req_list: List[int] = []
        resp_list: List[int] = []
        if "req_hex" in f:
            req_list = _hex_to_bytes(str(f["req_hex"]))
        elif "req" in f:
            req_list = [int(b) & 0xFF for b in f["req"]]
        elif "tx_bytes" in f:
            req_list = [int(b) & 0xFF for b in f["tx_bytes"]]
        if "resp_hex" in f:
            resp_list = _hex_to_bytes(str(f["resp_hex"]))
        elif "resp" in f:
            resp_list = [int(b) & 0xFF for b in f["resp"]]
        elif "rx_bytes" in f:
            resp_list = [int(b) & 0xFF for b in f["rx_bytes"]]
        out.append(UartFrame(req=req_list, resp=resp_list))
    return out


# ---------- comparators ----------

def _cmp_strict(gen: List[UartFrame], golden: List[UartFrame]) -> Tuple[bool, str, Dict[str, Any]]:
    if len(gen) != len(golden):
        return False, (
            f"frame count differs: generated={len(gen)} golden={len(golden)}"
        ), {"generated_frames": len(gen), "golden_frames": len(golden)}
    for i, (g, ref) in enumerate(zip(gen, golden)):
        if g.req != ref.req:
            return False, (
                f"frame {i} request differs: gen={g.req!r} golden={ref.req!r}"
            ), {"first_diff_frame": i, "diff_field": "req"}
        if g.resp != ref.resp:
            return False, (
                f"frame {i} response differs: gen={g.resp!r} golden={ref.resp!r}"
            ), {"first_diff_frame": i, "diff_field": "resp"}
    return True, f"byte-exact over {len(gen)} frames", {"generated_frames": len(gen)}


def _cmp_semantic(gen: List[UartFrame], golden: List[UartFrame]) -> Tuple[bool, str, Dict[str, Any]]:
    """Same frame count & order; req must match; resp is verified only when
    the oracle provides a non-empty ref.resp."""
    if len(gen) != len(golden):
        return False, (
            f"frame count differs: generated={len(gen)} golden={len(golden)}"
        ), {"generated_frames": len(gen), "golden_frames": len(golden)}
    for i, (g, ref) in enumerate(zip(gen, golden)):
        if g.req != ref.req:
            return False, (
                f"frame {i} request differs: gen={g.req!r} golden={ref.req!r}"
            ), {"first_diff_frame": i, "diff_field": "req"}
        if ref.resp and g.resp != ref.resp:
            return False, (
                f"frame {i} response differs: gen={g.resp!r} golden={ref.resp!r}"
            ), {"first_diff_frame": i, "diff_field": "resp"}
    return True, f"semantic match over {len(gen)} frames", {"generated_frames": len(gen)}


def _cmp_relaxed(gen: List[UartFrame],
                 required: List[RequiredWrite]) -> Tuple[bool, str, Dict[str, Any]]:
    """Every required entry must appear as a prefix of some generated request."""
    missing: List[str] = []
    for rw in required:
        ok = False
        for f in gen:
            for any_bytes in rw.any_of:
                if f.req[: len(any_bytes)] == list(any_bytes):
                    ok = True
                    break
            if ok:
                break
        if not ok:
            missing.append(f"{rw.description or rw.any_of[0]!r}")
    if missing:
        return False, (
            f"required writes not seen: {missing}"
        ), {"missing": missing, "generated_frames": len(gen)}
    return True, (
        f"all {len(required)} required writes present in {len(gen)} frames"
    ), {"generated_frames": len(gen)}


# ---------- main judge ----------

def judge(
    device_id: str,
    vector_outcomes: Iterable[UartVectorOutcome],
    oracle: OracleData,
    *,
    policy: Policy = "auto",
) -> LevelVerdict:
    """Aggregate per-vector L3 verdicts into a single overall L3 verdict."""
    outcomes = list(vector_outcomes)
    resolved, reason = _resolve_policy(oracle, policy)
    claim = _claim_for(resolved)

    if resolved == "vacuous":
        return LevelVerdict(
            device=device_id, level="L3", passed=True,
            claim=claim, detail=reason,
            evidence={
                "policy": "vacuous",
                "requested_policy": policy,
                "vectors": len(outcomes),
                "bus_type": oracle.meta.bus_type,
            },
        )

    if resolved in ("strict", "semantic") and oracle.golden_trace is None:
        return LevelVerdict(
            device=device_id, level="L3", passed=False,
            claim=claim,
            detail=f"{resolved} policy requested but no golden_trace",
            evidence={"policy": resolved, "requested_policy": policy},
        )
    if resolved == "relaxed" and not oracle.required_writes:
        return LevelVerdict(
            device=device_id, level="L3", passed=True,
            claim="protocol-valid-relaxed",
            detail="relaxed policy, but required_writes is empty — vacuous",
            evidence={"policy": "vacuous_relaxed",
                      "requested_policy": policy,
                      "vectors": len(outcomes)},
        )
    if not outcomes:
        return LevelVerdict(
            device=device_id, level="L3", passed=False,
            claim=claim,
            detail="no vectors to evaluate",
            evidence={"policy": resolved, "requested_policy": policy},
        )

    golden = _golden_frames(oracle.golden_trace) if resolved in ("strict", "semantic") else []

    per_vector: List[dict] = []
    any_fail = False
    for out in outcomes:
        sub = _judge_one_vector(out, resolved, golden, oracle)
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
        claim=claim, detail=detail,
        evidence={
            "policy":           resolved,
            "requested_policy": policy,
            "total":            total,
            "passed":           pass_count,
            "failed_count":     total - pass_count,
            "per_vector":       per_vector[:16],
        },
    )


def _judge_one_vector(
    outcome: UartVectorOutcome,
    resolved_policy: str,
    golden: List[UartFrame],
    oracle: OracleData,
) -> dict:
    stim = outcome.stimulus_name
    if outcome.trace_path is None or not Path(outcome.trace_path).exists():
        return {"stimulus": stim, "passed": False,
                "detail": f"no trace file captured (path={outcome.trace_path})"}
    try:
        gen = _load_uart_trace(Path(outcome.trace_path))
    except Exception as e:
        return {"stimulus": stim, "passed": False,
                "detail": f"trace load error: {e!r}"}

    if resolved_policy == "strict":
        ok, detail, ev = _cmp_strict(gen, golden)
    elif resolved_policy == "semantic":
        ok, detail, ev = _cmp_semantic(gen, golden)
    elif resolved_policy == "relaxed":
        ok, detail, ev = _cmp_relaxed(gen, oracle.required_writes)
    else:
        return {"stimulus": stim, "passed": False,
                "detail": f"unknown policy {resolved_policy!r}"}

    ret = {
        "stimulus": stim,
        "passed":   bool(ok),
        "detail":   detail,
        "generated_frames": len(gen),
    }
    ret.update(ev)
    return ret


__all__ = ["judge", "Policy", "UartFrame"]
