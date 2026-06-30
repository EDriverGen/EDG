"""evaluation.orchestrator - single entry point for the evaluation ladder."""
from __future__ import annotations

import dataclasses
import json
import logging
import re
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Mapping, Optional, Tuple

from evaluation.ladder.l1_build import judge as judge_l1
from evaluation.ladder.l2_boot import judge as judge_l2
from evaluation.ladder.l3_protocol.i2c import (
    Policy as L3Policy,
    judge as judge_l3_i2c,
)
from evaluation.ladder.l3_protocol.spi import judge as judge_l3_spi
from evaluation.ladder.l3_protocol.uart import judge as judge_l3_uart
from evaluation.ladder.l3_protocol.gpio_pulse import judge as judge_l3_gpio
from evaluation.ladder.l4_semantic import judge as judge_l4
from evaluation.ladder.l5_robust.gpio_fault import judge as judge_l5_gpio
from evaluation.ladder.l5_robust.i2c_nack import judge as judge_l5_nack
from evaluation.ladder.l5_robust.spi_fault import judge as judge_l5_spi
from evaluation.ladder.l5_robust.uart_fault import judge as judge_l5_uart
from evaluation.models import EvaluationReport, LevelVerdict
from evaluation.oracle import ORACLE_DATA_DIR
from evaluation.oracle.loader import load_oracle
from evaluation.oracle.schema import OracleData
from evaluation.runtime.compile import CompileResult, link_mode_compile
from evaluation.runtime.i2c_runner import VectorOutcome, run_i2c_vectors
from evaluation.runtime.spi_runner import run_spi_vectors
from evaluation.runtime.uart_runner import run_uart_vectors
from evaluation.runtime.gpio_pulse_runner import run_gpio_vectors


log = logging.getLogger("evaluation.orchestrator")


LEVEL_CLAIM_ORDER: List[str] = [
    "build-valid",
    "runtime-smoke-valid",
    "protocol-valid-relaxed",
    "protocol-valid-semantic",
    "protocol-valid-strict",
    "semantic-valid",
    "robust-valid",
]


@dataclass
class OrchestratorOptions:
    """Knobs for evaluate()."""

    # Device / RTOS routing
    device_id: str
    rtos_id: str

    # Inputs
    driver_dir: Path
    adapter_path: Optional[Path] = None  # defaults to <driver_dir>/<device_id>_eval_adapter.c

    # Runtime
    bus_kind: str = "i2c"   # one of compile._BUS_CONFIG keys
    eval_class: Optional[str] = None  # fallback to oracle.meta.eval_class
    bus_instance: Optional[str] = None  # e.g. "i2c1"; default from compile.py

    # Budget
    compile_timeout: int = 180
    timeout_per_vector: int = 60
    sleep_s: int = 20
    l5_timeout_per_scenario: int = 60
    l5_representative_stimulus_index: int = 0
    l3_policy: L3Policy = "auto"

    # Output
    work_dir: Optional[Path] = None  # default: tempfile
    report_out: Optional[Path] = None  # optional JSON output path

    # Skip some steps
    skip_l3: bool = False
    skip_l4: bool = False
    skip_l5: bool = False

    # Oracle root (tests can override)
    oracle_root: Optional[Path] = None

    # Optional fixed task package. Evaluation remains oracle-driven for
    # stimuli/expected values, but GPIO runtime pin placement must match the
    # concrete board attachment the generated adapter was built for.
    task_package_path: Optional[Path] = None


@dataclass
class EvaluationRun:
    """All artefacts of one evaluate() invocation."""

    report: EvaluationReport
    compile_result: Optional[CompileResult] = None
    vector_outcomes: List[VectorOutcome] = field(default_factory=list)
    work_dir: Optional[Path] = None
    oracle: Optional[OracleData] = None


# ---------- helpers ----------

def _infer_adapter_path(driver_dir: Path, device_id: str) -> Optional[Path]:
    candidates = list(driver_dir.glob(f"{device_id}_eval_adapter.c"))
    if candidates:
        return candidates[0]
    # fallback: any *_eval_adapter.c under driver_dir
    fallback = list(driver_dir.glob("*_eval_adapter.c"))
    return fallback[0] if fallback else None


# Regex matches the `.eval_class = DRIVERGEN_EVAL_CLASS_<NAME>` line that
# `adapter_generator` emits inside the `drivergen_eval_meta` struct. We parse
# the adapter source directly (vs linking + running) because L1 needs the
# diagnostic before the link ever happens.
_ADAPTER_EVAL_CLASS_RE = re.compile(
    r"\.eval_class\s*=\s*DRIVERGEN_EVAL_CLASS_(?P<name>[A-Z_]+)"
)
_ADAPTER_CLASS_MAP = {
    "SINGLE_CHANNEL": "single_channel",
    "MULTI_CHANNEL":  "multi_channel",
    "MEMORY":         "memory",
    "DISPLAY":        "display",
    "RTC":            "rtc",
}


def _parse_adapter_eval_class(adapter_path: Path) -> Optional[str]:
    """Parse adapter source to extract the declared eval_class."""
    try:
        text = adapter_path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None
    m = _ADAPTER_EVAL_CLASS_RE.search(text)
    if not m:
        return None
    return _ADAPTER_CLASS_MAP.get(m.group("name"))


_GPIO_PORT_LETTERS = {"A": 0, "B": 1, "C": 2, "D": 3, "E": 4}
_GPIO_PORT_NAMES = "ABCDE"


def _gpio_port_label(port_index: int, pin_index: int) -> str:
    port = _GPIO_PORT_NAMES[int(port_index)] if 0 <= int(port_index) < 5 else "B"
    return f"P{port}{int(pin_index)}"


def _parse_gpio_port_index(value: Any) -> Optional[int]:
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return value
    if not isinstance(value, str):
        return None
    text = value.strip()
    if not text:
        return None
    m = re.fullmatch(r"(?:GPIO|PORT|P)?([A-E])", text, flags=re.IGNORECASE)
    if m:
        return _GPIO_PORT_LETTERS.get(m.group(1).upper())
    try:
        return int(text, 0)
    except ValueError:
        return None


def _parse_gpio_pin_ref(value: Any) -> Optional[Tuple[Optional[int], int]]:
    """Parse task-package GPIO labels into ``(port_index, pin_index)``."""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, Mapping):
        pin = value.get("pin") or value.get("pin_index") or value.get("number")
        port = value.get("port")
        if port is None:
            port = value.get("port_index")
        if port is None:
            port = value.get("gpio_port")
        if pin is None:
            return None
        try:
            pin_i = int(pin, 0) if isinstance(pin, str) else int(pin)
        except (TypeError, ValueError):
            return None
        return _parse_gpio_port_index(port), pin_i
    if isinstance(value, int):
        return None, value
    if not isinstance(value, str):
        return None
    text = value.strip()
    if not text:
        return None

    m = re.fullmatch(r"P([A-E])\s*[,]?\s*(\d{1,2})", text, flags=re.IGNORECASE)
    if m:
        port = _GPIO_PORT_LETTERS.get(m.group(1).upper())
        return port, int(m.group(2), 0)

    for pattern in (
        r"\bGPIO([A-E])\b\s*:?\s*GPIO_PIN_(\d{1,2})\b",
        r"\bPAL_LINE\(\s*GPIO([A-E])\s*,\s*(\d{1,2})\s*\)",
        r"\bGET_PIN\(\s*(?:GPIO)?([A-E])\s*,\s*(\d{1,2})\s*\)",
    ):
        m = re.search(pattern, text, flags=re.IGNORECASE)
        if m:
            port = _GPIO_PORT_LETTERS.get(m.group(1).upper())
            return port, int(m.group(2), 0)

    m = re.search(
        r"\bGPIO_PIN\(\s*(\d{1,2})\s*,\s*(\d{1,2})\s*\)",
        text,
        flags=re.IGNORECASE,
    )
    if m:
        return int(m.group(1), 0), int(m.group(2), 0)

    for pattern in (
        r"GPIO_PIN_(\d{1,2})\b",
        r"/dev/gpio(\d{1,2})\b",
        r"\bgpio[_-]?(\d{1,2})\b",
    ):
        m = re.search(pattern, text, flags=re.IGNORECASE)
        if m:
            return None, int(m.group(1), 0)
    try:
        return None, int(text, 0)
    except ValueError:
        return None


def _candidate_fixed_attachments(payload: Mapping[str, Any]) -> List[Mapping[str, Any]]:
    out: List[Mapping[str, Any]] = []
    direct = payload.get("fixed_attachment")
    if isinstance(direct, Mapping):
        out.append(direct)
    conn = payload.get("connection_binding")
    if isinstance(conn, Mapping) and isinstance(conn.get("fixed_attachment"), Mapping):
        out.append(conn["fixed_attachment"])
    fixed = payload.get("fixed_task_context")
    if isinstance(fixed, Mapping):
        fixed_conn = fixed.get("connection")
        if (
            isinstance(fixed_conn, Mapping)
            and isinstance(fixed_conn.get("fixed_attachment"), Mapping)
        ):
            out.append(fixed_conn["fixed_attachment"])
    return out


def _load_task_gpio_attachment(task_package_path: Optional[Path]) -> Optional[Mapping[str, Any]]:
    if task_package_path is None:
        return None
    path = Path(task_package_path)
    if not path.is_file():
        return None
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
    if not isinstance(payload, Mapping):
        return None
    for attachment in _candidate_fixed_attachments(payload):
        if any(
            k in attachment
            for k in (
                "data_pin",
                "data_line",
                "gpio_data_pin",
                "gpio_pin",
                "gpio_pin_number",
                "echo_pin",
                "echo_line",
                "trig_pin",
                "trig_line",
            )
        ):
            return attachment
    return None


def _apply_task_gpio_attachment(
    oracle: OracleData,
    task_package_path: Optional[Path],
) -> OracleData:
    """Align GPIO runtime pin placement with the fixed task package."""
    if oracle.meta.bus_type != "gpio":
        return oracle
    attachment = _load_task_gpio_attachment(task_package_path)
    if not attachment:
        return oracle

    updates: Dict[str, Any] = {}
    hint = str(getattr(oracle.meta, "gpio_protocol_hint", "") or "").lower()
    single_wire = "1-wire" in hint or "one-wire" in hint or "single" in hint

    data_ref = _parse_gpio_pin_ref(
        attachment.get("data_pin")
        or attachment.get("data_line")
        or attachment.get("gpio_data_pin")
        or attachment.get("gpio_pin")
    )
    if data_ref is None and "gpio_pin_number" in attachment:
        data_ref = _parse_gpio_pin_ref(
            {
                "port": attachment.get("gpio_port")
                or attachment.get("gpio_port_index"),
                "pin": attachment.get("gpio_pin_number"),
            }
        )

    if single_wire:
        if data_ref is None:
            return oracle
        data_port, data_pin = data_ref
        updates["gpio_pin_number"] = data_pin
        updates["gpio_trig_pin_number"] = -1
        updates["gpio_trig_port_index"] = -1
        if data_port is not None:
            updates["gpio_port_index"] = data_port
        return dataclasses.replace(
            oracle,
            meta=dataclasses.replace(oracle.meta, **updates),
        )

    echo_ref = _parse_gpio_pin_ref(
        attachment.get("echo_pin")
        or attachment.get("echo_line")
        or attachment.get("gpio_echo_pin")
    )
    trig_ref = _parse_gpio_pin_ref(
        attachment.get("trig_pin")
        or attachment.get("trig_line")
        or attachment.get("gpio_trig_pin")
    )
    if echo_ref is not None:
        echo_port, echo_pin = echo_ref
        updates["gpio_pin_number"] = echo_pin
        if echo_port is not None:
            updates["gpio_port_index"] = echo_port
    if trig_ref is not None:
        trig_port, trig_pin = trig_ref
        updates["gpio_trig_pin_number"] = trig_pin
        if trig_port is not None:
            updates["gpio_trig_port_index"] = trig_port
        elif "gpio_port_index" in updates:
            updates["gpio_trig_port_index"] = updates["gpio_port_index"]

    if not updates:
        return oracle
    return dataclasses.replace(
        oracle,
        meta=dataclasses.replace(oracle.meta, **updates),
    )


def _skipped_verdict(device_id: str, level: str,
                     claim: str, reason: str) -> LevelVerdict:
    return LevelVerdict(
        device=device_id, level=level, passed=False, claim=claim,
        detail=f"skipped ({reason})", evidence={"skipped": True},
    )


_LEVEL_ORDER: Dict[str, int] = {"L1": 1, "L2": 2, "L3": 3, "L4": 4, "L5": 5}


def _is_skipped_verdict(v: LevelVerdict) -> bool:
    """A verdict is "skipped" when the ladder aborted before this level."""
    ev = v.evidence or {}
    if bool(ev.get("skipped")):
        return True
    return str(v.detail or "").lstrip().lower().startswith("skipped")


def _overall_claim(verdicts: List[LevelVerdict]) -> str:
    """Strongest claim supported by the ladder, capped by any genuine downstream failure."""
    if not verdicts:
        return "none"
    ordered = sorted(verdicts, key=lambda v: _LEVEL_ORDER.get(v.level, 99))
    ranks = {c: i for i, c in enumerate(LEVEL_CLAIM_ORDER)}
    best = "none"
    for v in ordered:
        if v.passed:
            if ranks.get(v.claim, -1) > ranks.get(best, -1):
                best = v.claim
            continue
        if _is_skipped_verdict(v):
            continue
        break
    return best


# ---------- main entry ----------

def evaluate(opts: OrchestratorOptions) -> EvaluationRun:
    """Run the full L1-L5 ladder for a single driver bundle."""
    # -- Inputs
    oracle = load_oracle(opts.device_id, oracle_root=opts.oracle_root)
    oracle = _apply_task_gpio_attachment(oracle, opts.task_package_path)
    eval_class = opts.eval_class or oracle.meta.eval_class

    adapter_path = opts.adapter_path or _infer_adapter_path(
        opts.driver_dir, opts.device_id
    )
    if adapter_path is None or not adapter_path.exists():
        raise FileNotFoundError(
            f"adapter not found: expected <driver_dir>/{opts.device_id}_eval_adapter.c "
            f"under {opts.driver_dir}, and no <device>_eval_adapter.c fallback present"
        )

    # Check adapter/harness class compatibility before compiling.
    adapter_eval_class = _parse_adapter_eval_class(adapter_path)
    eval_class_mismatch: Optional[str] = None
    if adapter_eval_class and adapter_eval_class != eval_class:
        eval_class_mismatch = (
            f"adapter eval_class={adapter_eval_class!r} != "
            f"oracle/opts eval_class={eval_class!r} - adapter was generated for "
            f"a different device class, so the harness cannot link against it. "
            f"Fix: regenerate the adapter with the correct eval_class, or "
            f"update the classifier so driver synthesis matches the ground truth."
        )

    # -- Work dir
    work_dir = opts.work_dir or Path(tempfile.mkdtemp(prefix=f"dg_eval_{opts.device_id}_"))
    work_dir = Path(work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)
    build_dir = work_dir / "build"
    vectors_dir = work_dir / "vectors"
    l5_dir = work_dir / "l5"

    verdicts: List[LevelVerdict] = []

    # Compile -> L1
    log.info("evaluate[%s/%s]: compile", opts.device_id, opts.rtos_id)
    # -- For GPIO devices with separate trig/echo pins, derive bus_instance
    bus_instance = opts.bus_instance
    if bus_instance is None and opts.bus_kind == "gpio":
        trig_pin = getattr(oracle.meta, "gpio_trig_pin_number", -1)
        echo_pin = getattr(oracle.meta, "gpio_pin_number", 5)
        echo_port = getattr(oracle.meta, "gpio_port_index", 1)
        trig_port = getattr(oracle.meta, "gpio_trig_port_index", -1)
        if trig_port < 0:
            trig_port = echo_port
        if trig_pin >= 0 and (trig_pin != echo_pin or trig_port != echo_port):
            bus_instance = (
                f"{_gpio_port_label(trig_port, trig_pin)}:"
                f"{_gpio_port_label(echo_port, echo_pin)}"
            )
        elif (echo_port, echo_pin) != (1, 5):
            bus_instance = _gpio_port_label(echo_port, echo_pin)

    cr = link_mode_compile(
        driver_dir   = opts.driver_dir,
        adapter_path = adapter_path,
        eval_class   = eval_class,
        bus_kind     = opts.bus_kind,
        rtos_id      = opts.rtos_id,
        out_dir      = build_dir,
        device_id    = opts.device_id,
        bus_instance = bus_instance,
        timeout      = opts.compile_timeout,
    )
    # Preserve class-mismatch context in the L1 diagnostic.
    if eval_class_mismatch and cr.errors:
        cr = dataclasses.replace(
            cr,
            errors=[eval_class_mismatch, *cr.errors],
        )
    v_l1 = judge_l1(opts.device_id, cr)
    verdicts.append(v_l1)

    if not v_l1.passed:
        for lvl, claim in (
            ("L2", "runtime-smoke-valid"),
            ("L3", "protocol-valid-relaxed"),
            ("L4", "semantic-valid"),
            ("L5", "robust-valid"),
        ):
            verdicts.append(_skipped_verdict(opts.device_id, lvl, claim, "L1 failed"))
        return _finalise(
            opts, verdicts, compile_result=cr, vector_outcomes=[],
            work_dir=work_dir, oracle=oracle,
        )

    elf_path = cr.elf_path  # guaranteed by L1 pass

    # Run vectors -> L2
    log.info(
        "evaluate[%s/%s]: run %d stimuli (bus=%s)",
        opts.device_id, opts.rtos_id, len(oracle.stimuli), opts.bus_kind,
    )
    _runner_map = {
        "i2c": run_i2c_vectors,
        "spi": run_spi_vectors,
        "uart": run_uart_vectors,
        "gpio": run_gpio_vectors,
    }
    run_vectors_fn = _runner_map.get(opts.bus_kind)
    if run_vectors_fn is None:
        raise ValueError(f"unsupported bus_kind={opts.bus_kind!r} for L2+")
    outcomes = run_vectors_fn(
        elf_path, oracle, vectors_dir,
        timeout_per_vector=opts.timeout_per_vector,
        sleep_s=opts.sleep_s,
    )
    # Build expected-error map for L2 judge (stimuli with err field)
    expected_err_map = {
        s.name: True
        for s in oracle.stimuli
        if s.expected.err is not None
    }
    v_l2 = judge_l2(opts.device_id, outcomes, expected_err_map=expected_err_map or None)
    verdicts.append(v_l2)

    if not v_l2.passed:
        for lvl, claim in (
            ("L3", "protocol-valid-relaxed"),
            ("L4", "semantic-valid"),
            ("L5", "robust-valid"),
        ):
            verdicts.append(_skipped_verdict(opts.device_id, lvl, claim, "L2 failed"))
        return _finalise(
            opts, verdicts, compile_result=cr, vector_outcomes=outcomes,
            work_dir=work_dir, oracle=oracle,
        )

    # Evaluate L3, L4, and L5 independently.
    if opts.skip_l3:
        verdicts.append(_skipped_verdict(
            opts.device_id, "L3", "protocol-valid-relaxed", "skip_l3 set"))
    else:
        _l3_judge_map = {
            "i2c": lambda: judge_l3_i2c(opts.device_id, outcomes, oracle, policy=opts.l3_policy),
            "spi": lambda: judge_l3_spi(opts.device_id, outcomes, oracle, policy=opts.l3_policy),
            "uart": lambda: judge_l3_uart(opts.device_id, outcomes, oracle, policy=opts.l3_policy),
            "gpio": lambda: judge_l3_gpio(opts.device_id, outcomes, oracle, policy=opts.l3_policy),
        }
        l3_fn = _l3_judge_map.get(opts.bus_kind)
        if l3_fn is not None:
            v_l3 = l3_fn()
            verdicts.append(v_l3)
        else:
            verdicts.append(_skipped_verdict(
                opts.device_id, "L3", "protocol-valid-relaxed",
                f"no L3 judge for bus_kind={opts.bus_kind}"))

    if opts.skip_l4:
        verdicts.append(_skipped_verdict(
            opts.device_id, "L4", "semantic-valid", "skip_l4 set"))
    else:
        v_l4 = judge_l4(opts.device_id, outcomes, oracle)
        verdicts.append(v_l4)

    if opts.skip_l5:
        verdicts.append(_skipped_verdict(
            opts.device_id, "L5", "robust-valid", "skip_l5 set"))
    else:
        _l5_judge_map = {
            "i2c":  judge_l5_nack,
            "spi":  judge_l5_spi,
            "uart": judge_l5_uart,
            "gpio": judge_l5_gpio,
        }
        l5_fn = _l5_judge_map.get(opts.bus_kind)
        if l5_fn is None:
            verdicts.append(_skipped_verdict(
                opts.device_id, "L5", "robust-valid",
                f"no L5 fault-injection backend for bus_kind={opts.bus_kind!r}"))
        else:
            v_l5 = l5_fn(
                opts.device_id, elf_path, oracle, l5_dir,
                representative_stimulus_index=opts.l5_representative_stimulus_index,
                timeout_per_scenario=opts.l5_timeout_per_scenario,
                sleep_s=opts.sleep_s,
            )
            verdicts.append(v_l5)

    return _finalise(
        opts, verdicts, compile_result=cr, vector_outcomes=outcomes,
        work_dir=work_dir, oracle=oracle,
    )


# ---------- report assembly ----------

def _finalise(
    opts: OrchestratorOptions,
    verdicts: List[LevelVerdict],
    *,
    compile_result: Optional[CompileResult],
    vector_outcomes: List[VectorOutcome],
    work_dir: Path,
    oracle: OracleData,
) -> EvaluationRun:
    combo = f"{opts.device_id}_{opts.rtos_id}"
    report = EvaluationReport(
        device=opts.device_id, combo=combo, verdicts=verdicts,
    )
    if opts.report_out is not None:
        _write_report_json(opts.report_out, report, opts)

    return EvaluationRun(
        report          = report,
        compile_result  = compile_result,
        vector_outcomes = vector_outcomes,
        work_dir        = work_dir,
        oracle          = oracle,
    )


def _write_report_json(
    out_path: Path,
    report: EvaluationReport,
    opts: OrchestratorOptions,
) -> None:
    """Persist a JSON report."""
    import json

    payload: Dict[str, Any] = {
        "device":        report.device,
        "rtos":          opts.rtos_id,
        "combo":         report.combo,
        "overall_claim": _overall_claim(report.verdicts),
        "verdicts":      [v.to_dict() for v in report.verdicts],
    }
    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def summarise(run: EvaluationRun) -> Dict[str, Any]:
    """Short machine-readable summary (for CI, dashboards)."""
    return {
        "device":        run.report.device,
        "combo":         run.report.combo,
        "overall_claim": _overall_claim(run.report.verdicts),
        "levels":        [
            {
                "level":  v.level,
                "passed": v.passed,
                "claim":  v.claim,
                "detail": v.detail,
            }
            for v in run.report.verdicts
        ],
    }


__all__ = [
    "OrchestratorOptions",
    "EvaluationRun",
    "evaluate",
    "summarise",
]
