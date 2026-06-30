"""evaluation.ladder.l3_protocol.i2c - I2C protocol-valid judge."""
from __future__ import annotations

from typing import Iterable, List, Literal, Optional, Tuple

from evaluation.ladder.l3_protocol._i2c_compare import (
    compare_protocol_equivalence as _compare_protocol_equivalence,
    compare_relaxed as _compare_relaxed,
    compare_semantic as _compare_semantic,
    compare_strict as _compare_strict,
)
from evaluation.models import LevelVerdict
from evaluation.oracle.schema import OracleData, RequiredWrite
from evaluation.runtime.i2c_runner import VectorOutcome
from evaluation.runtime.trace_io import load_jsonl_trace


Policy = Literal["auto", "strict", "semantic", "relaxed"]


# ---------- helpers ----------

def _required_writes_to_dicts(rw: List[RequiredWrite]) -> List[dict]:
    """Convert RequiredWrite dataclasses to the dict form accepts."""
    return [
        {
            "addr": r.addr,
            "any_of": [list(p) for p in r.any_of],
            "description": r.description,
        }
        for r in rw
    ]


def _resolve_policy(oracle: OracleData, requested: Policy) -> Tuple[str, str]:
    """Return (resolved_policy, reason_detail)."""
    if requested != "auto":
        return requested, f"policy={requested!r} (explicit)"
    if oracle.golden_trace is not None:
        if getattr(oracle.meta, "require_byte_exact", False):
            return "strict", (
                "auto-selected strict "
                "(meta.require_byte_exact=true, golden_trace present)"
            )
        return "semantic", "auto-selected semantic (golden_trace present)"
    if oracle.required_writes:
        return "relaxed", "auto-selected relaxed (required_writes present)"
    return "vacuous", "no oracle artifacts — L3 vacuously satisfied"


def _claim_for(policy: str) -> str:
    return {
        "strict":                   "protocol-valid-strict",
        "semantic":                 "protocol-valid-semantic",
        "relaxed":                  "protocol-valid-relaxed",
        "vacuous":                  "protocol-valid-relaxed",
        "semantic_equivalence":     "protocol-valid-semantic",
        "semantic_fallback_relaxed": "protocol-valid-relaxed",
    }[policy]


# ---------- aggregate runner (internal) ----------

def _run_aggregate(
    device_id: str,
    outcomes: List[VectorOutcome],
    policy_name: str,
    golden,
    required_writes: List[dict],
    protocol_equivalence=None,
) -> Tuple[bool, List[dict]]:
    """Run a single policy across all vectors; return (all_passed, per_vector)."""
    per_vector: List[dict] = []
    any_fail = False
    for out in outcomes:
        sub = _judge_one_vector(
            device_id,
            out,
            policy_name,
            golden,
            required_writes,
            protocol_equivalence=protocol_equivalence,
        )
        per_vector.append(sub)
        if not sub["passed"]:
            any_fail = True
    return (not any_fail, per_vector)


def _aggregate_detail(policy_name: str, per_vector: List[dict]) -> str:
    """Format the top-level detail string for an aggregate result."""
    pass_count = sum(1 for p in per_vector if p["passed"])
    total = len(per_vector)
    if pass_count == total:
        return f"{pass_count}/{total} vectors match oracle under {policy_name} policy"
    worst = next(p for p in per_vector if not p["passed"])
    return (
        f"{pass_count}/{total} vectors passed {policy_name}; "
        f"first failure: {worst['stimulus']} — {worst['detail']}"
    )


# ---------- main judge ----------

def judge(
    device_id: str,
    vector_outcomes: Iterable[VectorOutcome],
    oracle: OracleData,
    *,
    policy: Policy = "auto",
) -> LevelVerdict:
    """Aggregate per-vector L3 verdicts into a single overall L3 verdict."""
    outcomes = list(vector_outcomes)
    resolved, reason = _resolve_policy(oracle, policy)
    claim = _claim_for(resolved)

    # Vacuous pass when no oracle data at all.
    if resolved == "vacuous":
        return LevelVerdict(
            device=device_id, level="L3", passed=True,
            claim=claim,
            detail=reason,
            evidence={
                "policy": "vacuous",
                "requested_policy": policy,
                "vectors": len(outcomes),
            },
        )

    # Precondition: strict / semantic need a golden_trace.
    if resolved in ("strict", "semantic") and oracle.golden_trace is None:
        return LevelVerdict(
            device=device_id, level="L3", passed=False,
            claim=claim,
            detail=f"{resolved} policy requested but no golden_trace",
            evidence={"policy": resolved, "requested_policy": policy},
        )
    # Precondition: relaxed needs required_writes; if empty, mark vacuous.
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

    golden = _load_golden_trace(oracle) if resolved in ("strict", "semantic") else None
    required_writes = _required_writes_to_dicts(oracle.required_writes) \
        if oracle.required_writes else []

    # ---- primary pass under the resolved policy ----
    primary_policy = resolved
    primary_reqs = required_writes if primary_policy == "relaxed" else []
    primary_pass, primary_pv = _run_aggregate(
        device_id, outcomes, primary_policy, golden, primary_reqs,
    )
    primary_detail = _aggregate_detail(primary_policy, primary_pv)
    primary_count = sum(1 for p in primary_pv if p["passed"])
    total = len(primary_pv)

    # ---- auto-only configured semantic equivalence ----
    if (
        not primary_pass
        and policy == "auto"
        and primary_policy == "semantic"
        and oracle.protocol_equivalence
    ):
        equiv_pass, equiv_pv = _run_aggregate(
            device_id,
            outcomes,
            "semantic_equivalence",
            None,
            [],
            protocol_equivalence=oracle.protocol_equivalence,
        )
        if equiv_pass:
            equiv_detail = _aggregate_detail("semantic_equivalence", equiv_pv)
            equiv_count = sum(1 for p in equiv_pv if p["passed"])
            return LevelVerdict(
                device=device_id, level="L3", passed=True,
                claim=_claim_for("semantic_equivalence"),
                detail=(
                    f"semantic diverged from single golden trace but matched "
                    f"configured protocol_equivalence ({equiv_count}/"
                    f"{len(equiv_pv)} vectors); treating as "
                    f"protocol-valid-semantic. equivalence note: "
                    f"{equiv_detail}; semantic note: {primary_detail}"
                ),
                evidence={
                    "policy":              "semantic_equivalence",
                    "requested_policy":    policy,
                    "total":               total,
                    "passed":              equiv_count,
                    "failed_count":        0,
                    "per_vector":          equiv_pv[:16],
                    "semantic_detail":     primary_detail,
                    "semantic_per_vector": primary_pv[:16],
                    "semantic_passed":     primary_count,
                    "semantic_failed":     total - primary_count,
                    "equivalence_reason": (
                        "auto: semantic aggregate FAIL against a single "
                        "golden_trace path, but every vector matches the "
                        "configured protocol_equivalence rules"
                    ),
                },
            )

    # ---- auto-only semantic→relaxed fallback ----
    if (
        not primary_pass
        and policy == "auto"
        and primary_policy == "semantic"
        and oracle.required_writes
    ):
        relaxed_pass, relaxed_pv = _run_aggregate(
            device_id, outcomes, "relaxed", None, required_writes,
        )
        if relaxed_pass:
            relaxed_detail = _aggregate_detail("relaxed", relaxed_pv)
            relaxed_count = sum(1 for p in relaxed_pv if p["passed"])
            return LevelVerdict(
                device=device_id, level="L3", passed=True,
                claim=_claim_for("semantic_fallback_relaxed"),
                detail=(
                    f"semantic diverged from golden but all required_writes "
                    f"satisfied ({relaxed_count}/{len(relaxed_pv)} vectors "
                    f"under relaxed); downgrading to protocol-valid-relaxed. "
                    f"semantic note: {primary_detail}"
                ),
                evidence={
                    "policy":            "semantic_fallback_relaxed",
                    "requested_policy":  policy,
                    "total":             total,
                    "passed":            relaxed_count,
                    "failed_count":      0,
                    "per_vector":        relaxed_pv[:16],
                    "semantic_detail":   primary_detail,
                    "semantic_per_vector": primary_pv[:16],
                    "semantic_passed":   primary_count,
                    "semantic_failed":   total - primary_count,
                    "fallback_reason": (
                        "auto: semantic aggregate FAIL, relaxed aggregate "
                        "PASS using required_writes any_of; treating driver "
                        "as walking an alternative datasheet-legal opcode path"
                    ),
                },
            )
        # relaxed also failed → fall through and return the primary failure

    # ---- primary result is the final result ----
    return LevelVerdict(
        device=device_id, level="L3", passed=primary_pass,
        claim=claim,
        detail=primary_detail,
        evidence={
            "policy":           primary_policy,
            "requested_policy": policy,
            "total":            total,
            "passed":           primary_count,
            "failed_count":     total - primary_count,
            "per_vector":       primary_pv[:16],  # cap for report size
        },
    )


# ---------- per-vector dispatch ----------

def _judge_one_vector(
    device_id: str,
    outcome: VectorOutcome,
    resolved_policy: str,
    golden,
    required_writes: List[dict],
    protocol_equivalence=None,
) -> dict:
    """Return a dict summary for one vector's L3 outcome."""
    stim = outcome.stimulus_name
    # No trace file implies the run didn't capture any I2C activity — fail.
    if outcome.trace_path is None or not outcome.trace_path.exists():
        return {
            "stimulus": stim,
            "passed":   False,
            "detail":   f"no trace file captured (path={outcome.trace_path})",
        }

    try:
        gen = load_jsonl_trace(
            outcome.trace_path, device=device_id.upper(), source="generated"
        )
    except Exception as e:  # defensive — trace_io already swallows most errors
        return {
            "stimulus": stim, "passed": False,
            "detail": f"trace load error: {e!r}",
        }

    if resolved_policy == "strict":
        vd = _compare_strict(gen, golden)
    elif resolved_policy == "semantic":
        vd = _compare_semantic(gen, golden)
    elif resolved_policy == "semantic_equivalence":
        vd = _compare_protocol_equivalence(gen, protocol_equivalence or {})
    elif resolved_policy == "relaxed":
        vd = _compare_relaxed(gen, required=required_writes)
    else:
        return {
            "stimulus": stim, "passed": False,
            "detail":   f"unknown policy {resolved_policy!r}",
        }

    row: dict = {
        "stimulus":      stim,
        "passed":        bool(vd.passed),
        "detail":        vd.detail,
        "generated_len": vd.evidence.get("generated_len"),
        "txn_count":     len(gen.transactions),
    }
    # Surface policy-specific evidence fields so the aggregate report can
    # show missing/extra signatures (semantic) or missing required writes
    # (relaxed) without re-running the comparator.
    for key in (
        "missing_vs_golden", "extra_vs_golden",
        "missing_count", "extra_count", "golden_len",
        "missing",  # relaxed missing required_writes
        "rules_total", "passed_rules", "matches", "failed_rules",
    ):
        if key in vd.evidence:
            row[key] = vd.evidence[key]
    return row


def _load_golden_trace(oracle: OracleData) -> Optional[object]:
    """Coerce OracleData.golden_trace dict into an I2CTrace instance."""
    if oracle.golden_trace is None:
        return None
    from evaluation.models import I2CTrace
    return I2CTrace.from_dict(oracle.golden_trace)


__all__ = ["judge", "Policy"]
