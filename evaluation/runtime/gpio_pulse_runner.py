"""evaluation.runtime.gpio_pulse_runner - run GPIO-timing stimulus vectors."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional

from evaluation.infrastructure import PLATFORMS_DIR, project_relative_path, rebase_repl_paths
from evaluation.oracle.schema import OracleData, OracleMeta, Stimulus
from evaluation.runtime.renode_exec import (
    RenodeRunOutcome,
    run_self_running_firmware,
)
from evaluation.runtime.slave_renderer import (
    render_gpio_pulse_injector,
    render_ow_bitslot_injector,
)


# Dallas 1-Wire bit-slot devices (DS18B20 family) need the specialised
# bit-slot injector rather than the generic pulse-width playback model.
# Meta.gpio_protocol_hint selects the dispatch:
#   - "1-wire-bitslot" / "bitslot-1wire" -> ow_bitslot_injector
#   - anything else (including "1-wire" for DHT22) -> pulse injector
_BITSLOT_HINTS = {
    "1-wire-bitslot",
    "bitslot-1wire",
    "dallas_1wire_bitslot",
    "dallas-1wire-bitslot",
    "dallas_1wire",
    "dallas-1wire",
}


def _select_gpio_renderer(meta: OracleMeta):
    """Pick the slave renderer for a GPIO meta based on protocol hint."""
    hint = (meta.gpio_protocol_hint or "").strip().lower()
    if hint in _BITSLOT_HINTS:
        return render_ow_bitslot_injector
    return render_gpio_pulse_injector


# ---------- result model ----------

@dataclass
class GpioVectorOutcome:
    """Outcome of running one GPIO pulse-timing stimulus vector."""

    stimulus_name: str
    boot_detected: bool = False
    test_done: bool = False
    result_pass: bool = False
    result_err: bool = False
    read_raw: Optional[float] = None
    read_err: Optional[int] = None
    read_channels: Dict[str, float] = field(default_factory=dict)
    mem_bytes: List[int] = field(default_factory=list)
    mem_probe_addr: Optional[int] = None
    mem_probe_len: Optional[int] = None
    memory_size_bytes: Optional[int] = None
    memory_page_bytes: Optional[int] = None
    display_frame_len: Optional[int] = None
    display_frame_err: Optional[int] = None
    display_status_err: Optional[int] = None
    display_status: Optional[int] = None
    rtc_get_err: Optional[int] = None
    rtc_set_err: Optional[int] = None
    rtc_time: Dict[str, int] = field(default_factory=dict)
    trace_path: Optional[Path] = None
    output_lines: List[str] = field(default_factory=list)
    error: str = ""
    duration_s: float = 0.0
    renode_exit: Optional[int] = None

    @property
    def any_error(self) -> bool:
        return bool(self.error)


# ---------- .repl patching ----------

# STM32F103 GPIO banks used by the evaluation platform.  RT-Thread's
# GET_PIN(A, n) convention maps A/B/C to 0/1/2, matching these indices.
_GPIO_PORT_LETTERS = "ABCDE"
_GPIO_PORT_BASE = {
    0: "0x40010800",
    1: "0x40010C00",
    2: "0x40011000",
    3: "0x40011400",
    4: "0x40011800",
}
_GPIO_PORT_BLOCK_RE = re.compile(
    r"(?ms)^gpioPort(?P<letter>[A-E]):\s*"
    r"(?P<kind>Python\.PythonPeripheral|GPIOPort\.STM32F1GPIOPort)"
    r"\s*@\s*sysbus\s*"
    r"(?P<addr><0x[0-9A-Fa-f]+,\s*\+0x[0-9A-Fa-f]+>|0x[0-9A-Fa-f]+)"
    r".*?(?=^\w[\w.]*:|\Z)"
)
_GPIO_PORTB_FILENAME_RE = re.compile(
    r'(gpioPortB:\s*Python\.PythonPeripheral[^{]*?filename:\s*")[^"]*(")',
    re.DOTALL,
)


def _gpio_port_letter(port_index: int) -> str:
    if not (0 <= int(port_index) < len(_GPIO_PORT_LETTERS)):
        raise RuntimeError(
            f"unsupported gpio_port_index={port_index!r}; expected 0..4 "
            "(A..E)"
        )
    return _GPIO_PORT_LETTERS[int(port_index)]


def _gpio_block_base(port_index: int, addr_text: str) -> str:
    m = re.search(r"0x[0-9A-Fa-f]+", addr_text or "")
    if m:
        return m.group(0)
    return _GPIO_PORT_BASE.get(int(port_index), "0x40010C00")


def _python_gpio_block(letter: str, base: str, slave_str: str) -> str:
    return (
        f"gpioPort{letter}: Python.PythonPeripheral @ sysbus {base}\n"
        "    size: 0x400\n"
        "    initable: true\n"
        f"    filename: \"{slave_str}\"\n"
    )


def _stm32_gpio_block(letter: str, base: str) -> str:
    return (
        f"gpioPort{letter}: GPIOPort.STM32F1GPIOPort @ sysbus <{base}, +0x400>\n"
        "    [0-15] -> exti@[0-15]\n"
    )


def _render_vector_repl(
    base_repl_text: str,
    slave_path: Path,
    out_path: Path,
    *,
    gpio_port_index: int = 1,
) -> Path:
    """Patch the base .repl to attach the slave to the selected GPIO bank."""
    base_repl_text = rebase_repl_paths(base_repl_text)
    slave_str = project_relative_path(slave_path)
    target_letter = _gpio_port_letter(gpio_port_index)

    seen_target = False

    def repl(match: re.Match[str]) -> str:
        nonlocal seen_target
        letter = match.group("letter")
        port_index = _GPIO_PORT_LETTERS.index(letter)
        base = _gpio_block_base(port_index, match.group("addr"))
        if letter == target_letter:
            seen_target = True
            return _python_gpio_block(letter, base, slave_str)
        # The stock GPIO platform has GPIOB as a Python peripheral. If a
        # task binds echo/data to PA or PC, restore non-target GPIOB to a
        # normal STM32 port so only one pulse injector owns GPIO IDR/ODR.
        if match.group("kind") == "Python.PythonPeripheral":
            return _stm32_gpio_block(letter, base)
        return match.group(0)

    new_repl = _GPIO_PORT_BLOCK_RE.sub(repl, base_repl_text)
    if not seen_target:
        raise RuntimeError(
            f"failed to patch gpioPort{target_letter} in .repl; expected a "
            "`gpioPortX: GPIOPort.STM32F1GPIOPort` or "
            "`gpioPortX: Python.PythonPeripheral` entry"
        )

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(new_repl, encoding="utf-8")
    return out_path


# ---------- .resc synthesis ----------

def _render_vector_resc(
    elf_path: Path,
    repl_path: Path,
    device_id: str,
    out_path: Path,
    *,
    sleep_s: int = 20,
) -> Path:
    platform = project_relative_path(repl_path)
    elf = project_relative_path(elf_path)

    name = f"{device_id}_eval"
    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(
        f":name: {name}\n"
        f":description: GPIO pulse-timing vector test for {device_id}\n\n"
        f"$bin = @{elf}\n\n"
        f'mach create "{name}"\n'
        f"machine LoadPlatformDescription @{platform}\n\n"
        f"sysbus LoadELF $bin\n\n"
        f"showAnalyzer usart2\n\n"
        f"start\n"
        f"sleep {sleep_s}\n\n"
        f"pause\n"
        f"quit\n",
        encoding="utf-8",
    )
    return out_path


# ---------- outcome conversion ----------

def _outcome_from_renode(
    stim_name: str, run: RenodeRunOutcome
) -> GpioVectorOutcome:
    return GpioVectorOutcome(
        stimulus_name=stim_name,
        boot_detected=run.boot_detected,
        test_done=run.test_done,
        result_pass=run.result_pass,
        result_err=run.result_err,
        read_raw=run.read_raw,
        read_err=run.read_err,
        read_channels=dict(run.read_channels),
        mem_bytes=list(run.mem_bytes),
        mem_probe_addr=run.mem_probe_addr,
        mem_probe_len=run.mem_probe_len,
        memory_size_bytes=run.memory_size_bytes,
        memory_page_bytes=run.memory_page_bytes,
        display_frame_len=run.display_frame_len,
        display_frame_err=run.display_frame_err,
        display_status_err=run.display_status_err,
        display_status=run.display_status,
        rtc_get_err=run.rtc_get_err,
        rtc_set_err=run.rtc_set_err,
        rtc_time=dict(run.rtc_time),
        trace_path=run.trace_path,
        output_lines=run.output_lines,
        error=run.error,
        duration_s=run.duration_s,
        renode_exit=run.renode_exit,
    )


# ---------- single-vector runner ----------

def run_gpio_vector(
    elf_path: Path,
    meta: OracleMeta,
    stim: Stimulus,
    work_dir: Path,
    *,
    timeout: int = 60,
    sleep_s: int = 20,
    base_repl_path: Optional[Path] = None,
) -> GpioVectorOutcome:
    """Run a single GPIO pulse-timing stimulus vector in Renode."""
    work_dir = Path(work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    safe_name = re.sub(r"[^\w\-]", "_", stim.name)

    # 1. Render slave. Dispatch based on meta.gpio_protocol_hint so Dallas
    #    1-Wire bit-slot devices (DS18B20) use the specialised injector
    #    while DHT-style pulse-width devices stay on the generic one.
    slave_path = work_dir / f"gpio_slave_{safe_name}.py"
    renderer = _select_gpio_renderer(meta)
    try:
        renderer(meta, stim, slave_path)
    except Exception as e:
        return GpioVectorOutcome(
            stimulus_name=stim.name,
            error=f"slave render failed: {e}",
        )

    # 2. Patch .repl
    base_repl = base_repl_path or (PLATFORMS_DIR / "stm32f103_hw_gpio.repl")
    if not base_repl.exists():
        return GpioVectorOutcome(
            stimulus_name=stim.name,
            error=f"base .repl not found: {base_repl}",
        )
    try:
        gpio_port = int(getattr(meta, "gpio_port_index", 1))
        gpio_trig_port = int(getattr(meta, "gpio_trig_port_index", -1))
        if (
            getattr(meta, "gpio_trig_pin_number", -1) >= 0
            and gpio_trig_port >= 0
            and gpio_trig_port != gpio_port
        ):
            return GpioVectorOutcome(
                stimulus_name=stim.name,
                error=(
                    "gpio pulse runner cannot model trigger and echo on "
                    "different GPIO ports yet: "
                    f"echo_port={gpio_port}, trig_port={gpio_trig_port}"
                ),
            )
        repl_text = base_repl.read_text(encoding="utf-8")
        repl_path = _render_vector_repl(
            repl_text,
            slave_path,
            work_dir / f"gpio_platform_{safe_name}.repl",
            gpio_port_index=gpio_port,
        )
    except Exception as e:
        return GpioVectorOutcome(
            stimulus_name=stim.name,
            error=f"repl patch failed: {e}",
        )

    # 3. Synthesize .resc
    resc_path = _render_vector_resc(
        elf_path, repl_path, meta.device_id,
        work_dir / f"gpio_vector_{safe_name}.resc",
        sleep_s=sleep_s,
    )

    # 4. Set up trace path
    trace_path = work_dir / f"gpio_trace_{safe_name}.jsonl"

    # 5. Run Renode with GPIO trace env var
    run = run_self_running_firmware(
        meta.device_id,
        resc=resc_path,
        timeout=timeout,
        trace_path=trace_path,
        trace_env_var="DRIVERGEN_GPIO_TRACE_PATH",
    )

    return _outcome_from_renode(stim.name, run)


# ---------- multi-vector runner ----------

def run_gpio_vectors(
    elf_path: Path,
    oracle: OracleData,
    work_dir: Path,
    *,
    timeout_per_vector: int = 60,
    sleep_s: int = 20,
    base_repl_path: Optional[Path] = None,
) -> List[GpioVectorOutcome]:
    """Run all stimuli for a GPIO pulse-timing device."""
    if oracle.meta.bus_type != "gpio":
        raise ValueError(
            f"gpio_pulse_runner requires bus_type=='gpio', got "
            f"{oracle.meta.bus_type!r}"
        )

    results: List[GpioVectorOutcome] = []
    for stim in oracle.stimuli:
        outcome = run_gpio_vector(
            elf_path,
            oracle.meta,
            stim,
            work_dir,
            timeout=timeout_per_vector,
            sleep_s=sleep_s,
            base_repl_path=base_repl_path,
        )
        results.append(outcome)

    return results


__all__ = [
    "GpioVectorOutcome",
    "run_gpio_vector",
    "run_gpio_vectors",
]
