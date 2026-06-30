"""evaluation.ladder.l5_robust.spi_fault - L5 SPI bus-fault judge."""
from __future__ import annotations

from pathlib import Path
from typing import List, Optional

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import OracleData
from evaluation.runtime.spi_runner import SpiVectorOutcome, run_spi_vector

from evaluation.ladder.l5_robust._common import (
    aggregate_verdict,
    build_no_stimuli_verdict,
    build_not_applicable_verdict,
    build_skip_verdict,
    classify_fault_outcome,
    fault_is_detectable,
    pick_stimulus,
    resolve_scenarios,
    with_env_var,
)


_ENV_KEY         = "DRIVERGEN_SPI_FAULT_FIRST_N"
_FAULT_KIND      = "MISO-stuck-high"
_FAULT_DETECT_KEY = "spi_miso_stuck"


def _run_with_fault(
    elf_path: Path,
    meta,
    stim,
    work_dir: Path,
    fault_first_n: int,
    *,
    timeout: int,
    sleep_s: int,
) -> SpiVectorOutcome:
    with with_env_var(_ENV_KEY, str(int(fault_first_n))):
        return run_spi_vector(
            elf_path, meta, stim, work_dir,
            timeout=timeout, sleep_s=sleep_s,
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
    _runner=None,
) -> LevelVerdict:
    """Run SPI fault scenarios and aggregate into one L5 verdict."""
    if oracle.meta.bus_type != "spi":
        return build_skip_verdict(
            device_id, "SPI fault judge called on non-SPI bus",
            oracle.meta.bus_type,
        )
    if not oracle.stimuli:
        return build_no_stimuli_verdict(device_id)

    if not fault_is_detectable(oracle, _FAULT_DETECT_KEY):
        # Device protocol has no in-band feature to distinguish a bogus
        # 0xFF readout from a legitimate full-scale sample (e.g. a 10-bit
        # passive ADC with no WHOAMI / status / checksum). No driver —
        # reference or generated — could legitimately claim to detect this
        # fault. Declare not-applicable rather than silently rewarding.
        return build_not_applicable_verdict(
            device_id, _FAULT_KIND,
            "device protocol has no in-band fault-detect feature "
            "(no WHOAMI / status / checksum); a 0xFF readout is "
            "indistinguishable from a valid full-scale sample",
        )

    scenarios = resolve_scenarios(oracle)
    stim = pick_stimulus(oracle, representative_stimulus_index)

    runner = _runner or _run_with_fault
    work_dir = Path(work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    per_scenario: List[dict] = []
    for sc in scenarios:
        sub_dir = work_dir / f"l5_{sc.name}"
        outcome = runner(
            elf_path, oracle.meta, stim, sub_dir, sc.nack_first_n,
            timeout=timeout_per_scenario, sleep_s=sleep_s,
        )
        passed, detail = classify_fault_outcome(
            outcome, sc.nack_first_n, fault_kind=_FAULT_KIND,
        )
        per_scenario.append({
            "scenario":        sc.name,
            "fault_first_n":   sc.nack_first_n,
            "passed":          passed,
            "detail":          detail,
            "stimulus":        stim.name,
            "boot_detected":   outcome.boot_detected,
            "test_done":       outcome.test_done,
            "result_pass":     outcome.result_pass,
            "read_raw":        outcome.read_raw,
            "duration_s":      round(outcome.duration_s, 2),
        })

    return aggregate_verdict(device_id, stim.name, per_scenario, _FAULT_KIND)


__all__ = ["judge"]
