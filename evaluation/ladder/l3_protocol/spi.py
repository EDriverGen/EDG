"""evaluation.ladder.l3_protocol.spi - SPI protocol-valid judge."""
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Literal, Optional, Tuple

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import OracleData, OracleMeta, RequiredWrite
from evaluation.runtime.spi_runner import SpiVectorOutcome

Policy = Literal["auto", "strict", "semantic", "relaxed"]


# ---------- trace primitives ----------

@dataclass(frozen=True)
class SpiFrame:
    """One CS-delimited frame captured by the generic SPI slave."""
    seq: int
    proto: str
    tx_bytes: List[int]
    rx_bytes: List[int]


def _load_spi_trace(path: Path) -> List[SpiFrame]:
    """Parse a JSONL SPI trace into a list of SpiFrame."""
    out: List[SpiFrame] = []
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
            out.append(SpiFrame(
                seq=int(obj.get("seq", 0)),
                proto=str(obj.get("proto", "register")),
                tx_bytes=[int(b) & 0xFF for b in obj.get("tx_bytes", [])],
                rx_bytes=[int(b) & 0xFF for b in obj.get("rx_bytes", [])],
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


# ---------- golden-trace parsing ----------

def _golden_frames(raw: Optional[Dict[str, Any]]) -> List[SpiFrame]:
    """Convert an oracle golden_trace dict into a list of SpiFrame."""
    if not raw:
        return []
    frames_raw = raw.get("frames")
    if frames_raw is None:
        frames_raw = raw.get("transactions", [])
    out: List[SpiFrame] = []
    for i, f in enumerate(frames_raw):
        try:
            out.append(SpiFrame(
                seq=int(f.get("seq", i)),
                proto=str(f.get("proto", "register")),
                tx_bytes=[int(b) & 0xFF for b in f.get("tx_bytes", [])],
                rx_bytes=[int(b) & 0xFF for b in f.get("rx_bytes", [])],
            ))
        except (KeyError, ValueError, TypeError):
            continue
    return out


# ---------- register-proto decoding for semantic policy ----------

def _decode_register_cmd(first_byte: int, meta: OracleMeta) -> Tuple[bool, bool, int]:
    """Decode a SPI register-proto command byte into (read, mb, reg_addr)."""
    rw_set = bool(first_byte & meta.spi_rw_mask)
    is_read = rw_set if meta.spi_read_when_set else (not rw_set)
    is_mb = bool(first_byte & meta.spi_mb_mask) if meta.spi_mb_mask else False
    reg = first_byte & meta.spi_addr_mask
    return is_read, is_mb, reg


def _frame_signature(frame: SpiFrame, meta: OracleMeta) -> Tuple:
    """Command-level signature used by semantic policy."""
    if frame.proto == "register" and frame.tx_bytes:
        is_read, _mb, reg = _decode_register_cmd(frame.tx_bytes[0], meta)
        burst = max(0, len(frame.tx_bytes) - 1)
        # For writes we keep the payload in the signature; for reads the
        # payload is dominated by 0x00 pad and is uninteresting.
        if is_read:
            payload: Tuple[int, ...] = ()
        else:
            payload = tuple(frame.tx_bytes[1:])
        return ("register", is_read, reg, burst, payload)
    return (frame.proto, tuple(frame.tx_bytes))


# ---------- per-policy per-vector comparators ----------

def _cmp_strict(gen: List[SpiFrame], golden: List[SpiFrame]) -> Tuple[bool, str, Dict[str, Any]]:
    if len(gen) != len(golden):
        return False, (
            f"frame count differs: generated={len(gen)} golden={len(golden)}"
        ), {"generated_frames": len(gen), "golden_frames": len(golden)}
    for i, (g, ref) in enumerate(zip(gen, golden)):
        if g.tx_bytes != ref.tx_bytes:
            return False, (
                f"frame {i} tx_bytes differ: gen={g.tx_bytes!r} golden={ref.tx_bytes!r}"
            ), {"first_diff_frame": i, "diff_field": "tx_bytes"}
        if g.rx_bytes != ref.rx_bytes:
            return False, (
                f"frame {i} rx_bytes differ: gen={g.rx_bytes!r} golden={ref.rx_bytes!r}"
            ), {"first_diff_frame": i, "diff_field": "rx_bytes"}
    return True, f"byte-exact over {len(gen)} frames", {"generated_frames": len(gen)}


def _cmp_semantic(gen: List[SpiFrame], golden: List[SpiFrame],
                  meta: OracleMeta) -> Tuple[bool, str, Dict[str, Any]]:
    if len(gen) != len(golden):
        return False, (
            f"frame count differs: generated={len(gen)} golden={len(golden)}"
        ), {"generated_frames": len(gen), "golden_frames": len(golden)}
    for i, (g, ref) in enumerate(zip(gen, golden)):
        if _frame_signature(g, meta) != _frame_signature(ref, meta):
            return False, (
                f"frame {i} command signature differs: "
                f"gen={_frame_signature(g, meta)!r} golden={_frame_signature(ref, meta)!r}"
            ), {"first_diff_frame": i, "diff_field": "signature"}
    return True, f"semantic match over {len(gen)} frames", {"generated_frames": len(gen)}


def _cmp_relaxed(gen: List[SpiFrame],
                 required: List[RequiredWrite]) -> Tuple[bool, str, Dict[str, Any]]:
    """Every required entry must appear as a tx prefix in some frame."""
    missing: List[str] = []
    for rw in required:
        ok = False
        for f in gen:
            for any_bytes in rw.any_of:
                if f.tx_bytes[: len(any_bytes)] == list(any_bytes):
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
    vector_outcomes: Iterable[SpiVectorOutcome],
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
        claim=claim,
        detail=detail,
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
    outcome: SpiVectorOutcome,
    resolved_policy: str,
    golden: List[SpiFrame],
    oracle: OracleData,
) -> dict:
    stim = outcome.stimulus_name
    if outcome.trace_path is None or not Path(outcome.trace_path).exists():
        return {
            "stimulus": stim, "passed": False,
            "detail": f"no trace file captured (path={outcome.trace_path})",
        }
    try:
        gen = _load_spi_trace(Path(outcome.trace_path))
    except Exception as e:  # defensive — _load_spi_trace already swallows most
        return {"stimulus": stim, "passed": False,
                "detail": f"trace load error: {e!r}"}

    if resolved_policy == "strict":
        ok, detail, ev = _cmp_strict(gen, golden)
    elif resolved_policy == "semantic":
        ok, detail, ev = _cmp_semantic(gen, golden, oracle.meta)
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


__all__ = ["judge", "Policy", "SpiFrame"]
