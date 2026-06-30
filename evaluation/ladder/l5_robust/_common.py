"""evaluation.ladder.l5_robust._common - shared helpers for all bus-specific L5 fault-injection backends."""
from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, List, Optional, Protocol, Tuple

from evaluation.models import LevelVerdict
from evaluation.oracle.schema import NackScenario, OracleData, Stimulus


class OutcomeLike(Protocol):
    """Minimal surface the classifier needs. Matches VectorOutcome /
    SpiVectorOutcome / UartVectorOutcome / GpioVectorOutcome."""
    boot_detected: bool
    test_done: bool
    result_pass: bool
    error: str
    duration_s: float
    read_raw: Optional[float]

    @property
    def any_error(self) -> bool: ...  # pragma: no cover


def classify_fault_outcome(
    outcome: OutcomeLike,
    fault_first_n: int,
    *,
    fault_kind: str,
) -> Tuple[bool, str]:
    """Generic outcome classifier for any fault-injection scenario."""
    if outcome.any_error:
        return False, f"renode infra error: {outcome.error}"
    if not outcome.boot_detected:
        return False, f"firmware never booted under {fault_kind} injection"
    if not outcome.test_done:
        return False, (
            f"firmware booted but DRIVER_TEST DONE never printed under "
            f"{fault_kind} injection (likely hung in driver error path)"
        )
    if outcome.result_pass:
        return False, (
            f"driver reported RESULT: PASS despite first {fault_first_n} "
            f"{fault_kind} event(s) — missing error check"
        )
    return True, (
        f"{fault_kind}×{fault_first_n}: booted + done + reported failure "
        "(not PASS)"
    )


def with_env_var(env_key: str, value: str) -> "_EnvSwap":
    """Context manager that temporarily sets ``env_key`` to ``value`` and
    restores the prior value (or absence) on exit.
    """
    return _EnvSwap(env_key, value)


@dataclass
class _EnvSwap:
    key: str
    value: str
    _prev: Optional[str] = None

    def __enter__(self) -> None:
        self._prev = os.environ.get(self.key)
        os.environ[self.key] = self.value

    def __exit__(self, exc_type, exc, tb) -> None:
        if self._prev is None:
            os.environ.pop(self.key, None)
        else:
            os.environ[self.key] = self._prev


@dataclass
class BaseFaultJudgeContext:
    """Common inputs threaded through every bus-specific fault judge."""
    device_id: str
    elf_path: Path
    oracle: OracleData
    work_dir: Path
    representative_stimulus_index: int = 0
    timeout_per_scenario: int = 60
    sleep_s: int = 20


def pick_stimulus(oracle: OracleData, idx: int) -> Stimulus:
    """Pick the representative stimulus, gracefully falling back to 0."""
    try:
        return oracle.stimuli[idx]
    except IndexError:
        return oracle.stimuli[0]


def resolve_scenarios(
    oracle: OracleData,
) -> List[NackScenario]:
    """Return fault scenarios to run; falls back to the default transient/persistent pair if the oracle has none configured."""
    if oracle.nack_scenarios:
        return list(oracle.nack_scenarios)
    # Inline default mirrors oracle.schema.default_nack_scenarios.
    return [
        NackScenario(name="transient_fault_1", nack_first_n=1,
                     description="single transient bus fault at start"),
        NackScenario(name="persistent_fault_99", nack_first_n=99,
                     description="persistent bus fault on every transaction"),
    ]


def build_skip_verdict(
    device_id: str, reason: str, bus_type: str,
) -> LevelVerdict:
    """Build a `passed=False, skipped` verdict for a bus we can't run."""
    return LevelVerdict(
        device=device_id, level="L5", passed=False, claim="robust-valid",
        detail=f"{reason} — skipped (bus_type={bus_type!r})",
        evidence={"skipped": True, "reason": reason, "bus_type": bus_type},
    )


def build_no_stimuli_verdict(device_id: str) -> LevelVerdict:
    return LevelVerdict(
        device=device_id, level="L5", passed=False, claim="robust-valid",
        detail="oracle has no stimuli to run fault injection against",
        evidence={"skipped": False, "reason": "no_stimuli"},
    )


def build_not_applicable_verdict(
    device_id: str, fault_kind: str, reason: str,
) -> LevelVerdict:
    """Build a PASSED verdict for a device whose bus protocol has no in-band feature to distinguish the given fault from valid data."""
    return LevelVerdict(
        device=device_id, level="L5", passed=True, claim="robust-valid",
        detail=(
            f"{fault_kind} not applicable: {reason}"
        ),
        evidence={
            "not_applicable": True,
            "fault_kind":     fault_kind,
            "reason":         reason,
        },
    )


def fault_is_detectable(oracle: OracleData, fault_key: str) -> bool:
    """Consult oracle.meta.fault_detect[fault_key]; True if missing."""
    return bool(oracle.meta.fault_detect.get(fault_key, True))


def aggregate_verdict(
    device_id: str,
    stim_name: str,
    per_scenario: List[dict],
    fault_kind: str,
) -> LevelVerdict:
    """Assemble the final L5 LevelVerdict from per-scenario results."""
    passed_count = sum(1 for r in per_scenario if r["passed"])
    total = len(per_scenario)
    all_passed = passed_count == total

    if all_passed:
        detail = (f"{passed_count}/{total} {fault_kind} scenarios passed "
                  f"(stim={stim_name})")
    else:
        worst = next(r for r in per_scenario if not r["passed"])
        detail = (f"{passed_count}/{total} {fault_kind} scenarios passed; "
                  f"first failure: {worst['scenario']} — {worst['detail']}")

    return LevelVerdict(
        device=device_id, level="L5", passed=all_passed, claim="robust-valid",
        detail=detail,
        evidence={
            "scenarios":    per_scenario,
            "total":        total,
            "passed":       passed_count,
            "stimulus":     stim_name,
            "fault_kind":   fault_kind,
        },
    )


__all__ = [
    "OutcomeLike",
    "classify_fault_outcome",
    "with_env_var",
    "BaseFaultJudgeContext",
    "pick_stimulus",
    "resolve_scenarios",
    "build_skip_verdict",
    "build_no_stimuli_verdict",
    "build_not_applicable_verdict",
    "fault_is_detectable",
    "aggregate_verdict",
]
