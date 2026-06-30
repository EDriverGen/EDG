"""evaluation.ladder.l2_boot - L2 runtime-smoke-valid judge."""
from __future__ import annotations

from typing import Dict, Iterable, List, Optional

from evaluation.models import LevelVerdict
from evaluation.runtime.i2c_runner import VectorOutcome


def judge(
    device_id: str,
    vector_outcomes: Iterable[VectorOutcome],
    expected_err_map: Optional[Dict[str, bool]] = None,
) -> LevelVerdict:
    """Produce an L2 verdict from all per-vector outcomes."""
    outcomes = list(vector_outcomes)
    total = len(outcomes)
    err_map = expected_err_map or {}

    if total == 0:
        return LevelVerdict(
            device=device_id, level="L2", passed=False,
            claim="runtime-smoke-valid",
            detail="no vectors to evaluate",
            evidence={"total": 0, "passed": 0, "failed": []},
        )

    failed: List[dict] = []
    for o in outcomes:
        expects_err = err_map.get(o.stimulus_name, False)
        reasons = []
        if o.any_error:
            reasons.append(f"error={o.error!r}")
        if not o.boot_detected:
            reasons.append("no_boot")
        if not o.test_done:
            reasons.append("no_done")
        # Accept RESULT: PASS, or RESULT: ERR when error is expected
        if not o.result_pass:
            if not (expects_err and getattr(o, 'result_err', False)):
                reasons.append("no_pass")
        if reasons:
            failed.append({
                "stimulus":  o.stimulus_name,
                "reasons":   reasons,
                "read_raw":  o.read_raw,
                "duration_s": round(o.duration_s, 2),
            })

    pass_count = total - len(failed)
    passed = (pass_count == total)

    if passed:
        detail = f"{pass_count}/{total} vectors boot+done+PASS"
    else:
        worst = failed[0]
        detail = (
            f"{pass_count}/{total} vectors passed; "
            f"first failure: {worst['stimulus']} ({', '.join(worst['reasons'])})"
        )

    return LevelVerdict(
        device=device_id, level="L2", passed=passed,
        claim="runtime-smoke-valid",
        detail=detail,
        evidence={
            "total":         total,
            "passed":        pass_count,
            "failed_count":  len(failed),
            "failed":        failed[:10],
            "passed_names":  [
                o.stimulus_name for o in outcomes
                if o.stimulus_name not in {f["stimulus"] for f in failed}
            ],
        },
    )


__all__ = ["judge"]
