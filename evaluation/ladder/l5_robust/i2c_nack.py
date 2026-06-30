"""evaluation.ladder.l5_robust.i2c_nack - L5 NACK-injection judge."""
from __future__ import annotations

import os
from pathlib import Path
from typing import List, Optional

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import OracleData, Stimulus
from evaluation.runtime.i2c_runner import VectorOutcome, run_i2c_vector

from evaluation.ladder.l5_robust._common import (
    build_not_applicable_verdict,
    fault_is_detectable,
)


_ENV_KEY          = "DRIVERGEN_I2C_NACK_FIRST_N"
_FAULT_KIND       = "NACK"
_FAULT_DETECT_KEY = "i2c_nack"


def _run_with_nack(
    elf_path: Path,
    meta,
    stim: Stimulus,
    work_dir: Path,
    nack_first_n: int,
    *,
    timeout: int,
    sleep_s: int,
) -> VectorOutcome:
    """Run one Renode vector with NACK injection active."""
    prev = os.environ.get(_ENV_KEY)
    os.environ[_ENV_KEY] = str(int(nack_first_n))
    try:
        return run_i2c_vector(
            elf_path, meta, stim, work_dir,
            timeout=timeout, sleep_s=sleep_s,
        )
    finally:
        if prev is None:
            os.environ.pop(_ENV_KEY, None)
        else:
            os.environ[_ENV_KEY] = prev


def _classify(outcome: VectorOutcome, nack_first_n: int) -> tuple[bool, str]:
    """Classify a NACK-injected outcome. Returns (passed, detail)."""
    if outcome.any_error:
        return False, f"renode infra error: {outcome.error}"
    if not outcome.boot_detected:
        return False, "firmware never booted under NACK injection"
    if not outcome.test_done:
        return False, (
            "firmware booted but DRIVER_TEST DONE never printed "
            "(likely hung in driver error path)"
        )
    if outcome.result_pass:
        return False, (
            f"driver reported RESULT: PASS despite first {nack_first_n} "
            "I2C transaction(s) being NACKed — missing error check"
        )
    return True, (
        f"NACK×{nack_first_n}: booted + done + reported failure (not PASS)"
    )


def judge(
    device_id: str,
    elf_path: Path,
    oracle: OracleData,
    work_dir: Path,
    *,
    representative_stimulus_index: int = 0,
    timeout_per_scenario: int = 60,
    sleep_s: int = 20,
    _runner=None,  # dependency injection for tests
) -> LevelVerdict:
    """Run every NACK scenario from ``oracle.nack_scenarios`` and aggregate."""
    if oracle.meta.bus_type not in ("i2c", "smbus"):
        return LevelVerdict(
            device=device_id, level="L5", passed=False, claim="robust-valid",
            detail=(f"NACK injection is I2C-specific; "
                    f"bus_type={oracle.meta.bus_type!r} — skipped"),
            evidence={
                "skipped":  True,
                "reason":   "non_i2c_bus",
                "bus_type": oracle.meta.bus_type,
            },
        )
    if not oracle.stimuli:
        return LevelVerdict(
            device=device_id, level="L5", passed=False, claim="robust-valid",
            detail="oracle has no stimuli to run NACK injection against",
            evidence={"skipped": False, "reason": "no_stimuli"},
        )
    if not fault_is_detectable(oracle, _FAULT_DETECT_KEY):
        return build_not_applicable_verdict(
            device_id, _FAULT_KIND,
            "driver cannot detect I2C NACK on this device "
            "(explicitly declared in oracle.meta.fault_detect)",
        )
    scenarios = list(oracle.nack_scenarios)
    if not scenarios:
        return LevelVerdict(
            device=device_id, level="L5", passed=False, claim="robust-valid",
            detail="oracle has no NACK scenarios configured",
            evidence={"skipped": False, "reason": "no_scenarios"},
        )

    try:
        stim = oracle.stimuli[representative_stimulus_index]
    except IndexError:
        stim = oracle.stimuli[0]

    runner = _runner or _run_with_nack
    work_dir = Path(work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    per_scenario: List[dict] = []
    any_fail = False
    for sc in scenarios:
        sub_dir = work_dir / f"l5_{sc.name}"
        outcome = runner(
            elf_path, oracle.meta, stim, sub_dir, sc.nack_first_n,
            timeout=timeout_per_scenario, sleep_s=sleep_s,
        )
        passed, detail = _classify(outcome, sc.nack_first_n)
        per_scenario.append({
            "scenario":      sc.name,
            "nack_first_n":  sc.nack_first_n,
            "passed":        passed,
            "detail":        detail,
            "stimulus":      stim.name,
            "boot_detected": outcome.boot_detected,
            "test_done":     outcome.test_done,
            "result_pass":   outcome.result_pass,
            "read_raw":      outcome.read_raw,
            "duration_s":    round(outcome.duration_s, 2),
        })
        if not passed:
            any_fail = True

    passed_count = sum(1 for r in per_scenario if r["passed"])
    total = len(per_scenario)
    all_passed = not any_fail

    if all_passed:
        detail = (f"{passed_count}/{total} NACK scenarios passed "
                  f"(stim={stim.name})")
    else:
        worst = next(r for r in per_scenario if not r["passed"])
        detail = (f"{passed_count}/{total} NACK scenarios passed; "
                  f"first failure: {worst['scenario']} — {worst['detail']}")

    return LevelVerdict(
        device=device_id, level="L5", passed=all_passed, claim="robust-valid",
        detail=detail,
        evidence={
            "scenarios":  per_scenario,
            "total":      total,
            "passed":     passed_count,
            "stimulus":   stim.name,
        },
    )


__all__ = ["judge"]
