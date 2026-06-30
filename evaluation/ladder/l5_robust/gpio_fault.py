"""evaluation.ladder.l5_robust.gpio_fault - L5 GPIO fault judge."""
from __future__ import annotations

from pathlib import Path
from typing import List, Optional

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import OracleData, OracleMeta
from evaluation.runtime.gpio_pulse_runner import (
    GpioVectorOutcome,
    run_gpio_vector,
)

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


# Same decision rule as gpio_pulse_runner._select_gpio_renderer —
# keep them in lockstep so the env var we set matches the slave that
# will actually be rendered.
_BITSLOT_HINTS = {"1-wire-bitslot", "bitslot-1wire"}


def _env_key_for(meta: OracleMeta) -> str:
    hint = (meta.gpio_protocol_hint or "").strip().lower()
    if hint in _BITSLOT_HINTS:
        return "DRIVERGEN_OW_FAULT_FIRST_N"
    return "DRIVERGEN_GPIO_FAULT_FIRST_N"


def _fault_kind_for(meta: OracleMeta) -> str:
    hint = (meta.gpio_protocol_hint or "").strip().lower()
    if hint in _BITSLOT_HINTS:
        return "no-presence-pulse"
    return "silent-GPIO-sensor"


def _fault_detect_key_for(meta: OracleMeta) -> str:
    hint = (meta.gpio_protocol_hint or "").strip().lower()
    if hint in _BITSLOT_HINTS:
        return "gpio_no_presence"
    return "gpio_silent"


def _run_with_fault(
    elf_path: Path,
    meta: OracleMeta,
    stim,
    work_dir: Path,
    fault_first_n: int,
    *,
    timeout: int,
    sleep_s: int,
) -> GpioVectorOutcome:
    env_key = _env_key_for(meta)
    with with_env_var(env_key, str(int(fault_first_n))):
        return run_gpio_vector(
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
    """Run GPIO fault scenarios and aggregate into one L5 verdict."""
    if oracle.meta.bus_type != "gpio":
        return build_skip_verdict(
            device_id, "GPIO fault judge called on non-GPIO bus",
            oracle.meta.bus_type,
        )
    if not oracle.stimuli:
        return build_no_stimuli_verdict(device_id)

    fault_kind       = _fault_kind_for(oracle.meta)
    fault_detect_key = _fault_detect_key_for(oracle.meta)
    if not fault_is_detectable(oracle, fault_detect_key):
        return build_not_applicable_verdict(
            device_id, fault_kind,
            "device protocol lacks an in-band fault-detect feature "
            "(no presence pulse / CRC / timeout edge) that a driver "
            "could use to distinguish a silent sensor from valid data",
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
            outcome, sc.nack_first_n, fault_kind=fault_kind,
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

    return aggregate_verdict(device_id, stim.name, per_scenario, fault_kind)


__all__ = ["judge"]
