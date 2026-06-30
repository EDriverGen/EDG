"""evaluation.runtime.i2c_runner - run I2C stimulus vectors in Renode."""
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
    render_i2c_display_slave,
    render_i2c_memory_slave,
    render_i2c_register_slave,
)


def _render_i2c_slave_for(meta: OracleMeta, stim: Stimulus, out_path: Path) -> Path:
    """Dispatch to the right slave renderer based on `meta.eval_class`."""
    if meta.eval_class == "memory":
        return render_i2c_memory_slave(meta, stim, out_path)
    if meta.eval_class == "display":
        return render_i2c_display_slave(meta, stim, out_path)
    return render_i2c_register_slave(meta, stim, out_path)


# ---------- result model ----------

@dataclass
class VectorOutcome:
    """Outcome of running one stimulus vector in Renode."""

    stimulus_name: str
    boot_detected: bool = False
    test_done: bool = False
    result_pass: bool = False
    result_err: bool = False
    read_raw: Optional[float] = None
    read_err: Optional[int] = None
    read_channels: Dict[str, float] = field(default_factory=dict)
    # memory class fields (populated when harness is test_main_memory)
    mem_bytes: List[int] = field(default_factory=list)
    mem_probe_addr: Optional[int] = None
    mem_probe_len: Optional[int] = None
    memory_size_bytes: Optional[int] = None
    memory_page_bytes: Optional[int] = None
    # display class fields (populated when harness is test_main_display)
    display_frame_len: Optional[int] = None
    display_frame_err: Optional[int] = None
    display_status_err: Optional[int] = None
    display_status: Optional[int] = None
    # rtc class fields (populated when harness is test_main_rtc)
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

# Regex matches the i2c1 Python.PythonPeripheral block's filename line,
# capturing everything up to and including `filename: "` and the closing `"`.
# Uses re.DOTALL so the `.*?` between "i2c1:" and "filename:" crosses lines.
_I2C1_FILENAME_RE = re.compile(
    r'(i2c1:.*?filename:\s*")[^"]*(")',
    re.DOTALL,
)


def _render_vector_repl(
    base_repl_text: str,
    slave_path: Path,
    out_path: Path,
) -> Path:
    """Patch the base .repl to point i2c1 at the rendered slave .py.

    Returns the resolved out_path.
    """
    # Rebase all filename: paths to the current project root (portability).
    base_repl_text = rebase_repl_paths(base_repl_text)

    # Renode.exe on Windows expects forward-slash paths.
    slave_str = project_relative_path(slave_path)

    new_repl = _I2C1_FILENAME_RE.sub(
        rf"\g<1>{slave_str}\2",
        base_repl_text,
    )
    if new_repl == base_repl_text:
        raise RuntimeError(
            "failed to patch i2c1 filename in .repl — "
            "regex did not match any Python.PythonPeripheral block"
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
    """Generate a .resc that loads the vector-specific .repl + ELF.

    Returns the resolved out_path.
    """
    platform = project_relative_path(repl_path)
    elf = project_relative_path(elf_path)

    name = f"{device_id}_eval"
    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(
        f":name: {name}\n"
        f":description: I2C vector test for {device_id}\n\n"
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


# ---------- single-vector runner ----------

def _outcome_from_renode(
    stim_name: str, run: RenodeRunOutcome
) -> VectorOutcome:
    """Convert a RenodeRunOutcome to a VectorOutcome."""
    return VectorOutcome(
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


def run_i2c_vector(
    elf_path: Path,
    meta: OracleMeta,
    stim: Stimulus,
    work_dir: Path,
    *,
    timeout: int = 60,
    sleep_s: int = 20,
    base_repl_path: Optional[Path] = None,
) -> VectorOutcome:
    """Run a single I2C stimulus vector in Renode."""
    work_dir = Path(work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    safe_name = re.sub(r"[^\w\-]", "_", stim.name)

    # 1. Render slave (dispatch on eval_class)
    slave_path = work_dir / f"slave_{safe_name}.py"
    try:
        _render_i2c_slave_for(meta, stim, slave_path)
    except Exception as e:
        return VectorOutcome(
            stimulus_name=stim.name,
            error=f"slave render failed: {e}",
        )

    # 2. Patch .repl
    base_repl = base_repl_path or (PLATFORMS_DIR / "stm32f103_hw_i2c.repl")
    if not base_repl.exists():
        return VectorOutcome(
            stimulus_name=stim.name,
            error=f"base .repl not found: {base_repl}",
        )
    try:
        repl_text = base_repl.read_text(encoding="utf-8")
        repl_path = _render_vector_repl(
            repl_text, slave_path, work_dir / f"platform_{safe_name}.repl"
        )
    except Exception as e:
        return VectorOutcome(
            stimulus_name=stim.name,
            error=f"repl patch failed: {e}",
        )

    # 3. Synthesize .resc
    resc_path = _render_vector_resc(
        elf_path, repl_path, meta.device_id,
        work_dir / f"vector_{safe_name}.resc",
        sleep_s=sleep_s,
    )

    # 4. Set up trace path
    trace_path = work_dir / f"trace_{safe_name}.jsonl"

    # 5. Run Renode
    run = run_self_running_firmware(
        meta.device_id,
        resc=resc_path,
        timeout=timeout,
        trace_path=trace_path,
    )

    return _outcome_from_renode(stim.name, run)


# ---------- multi-vector runner ----------

def run_i2c_vectors(
    elf_path: Path,
    oracle: OracleData,
    work_dir: Path,
    *,
    timeout_per_vector: int = 60,
    sleep_s: int = 20,
    base_repl_path: Optional[Path] = None,
) -> List[VectorOutcome]:
    """Run all stimuli for an I2C device, returning per-vector outcomes."""
    if oracle.meta.bus_type not in ("i2c", "smbus"):
        raise ValueError(
            f"i2c_runner requires bus_type in {{i2c, smbus}}, "
            f"got {oracle.meta.bus_type!r}"
        )

    results: List[VectorOutcome] = []
    for stim in oracle.stimuli:
        outcome = run_i2c_vector(
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
    "VectorOutcome",
    "run_i2c_vector",
    "run_i2c_vectors",
]
