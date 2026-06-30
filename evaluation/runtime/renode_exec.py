"""evaluation.runtime.renode_exec - thin wrapper around the `renode` CLI."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional

from evaluation.infrastructure import (
    PLATFORMS_DIR,  # noqa: F401  re-exported
    PROJECT_ROOT,
    project_relative_path,
)


@dataclass
class RenodeRunOutcome:
    renode_path: Optional[str] = None
    renode_exit: Optional[int] = None
    timed_out: bool = False
    error: str = ""
    boot_detected: bool = False
    test_done: bool = False
    result_pass: bool = False
    result_err: bool = False
    has_failures: bool = False
    read_raw: Optional[float] = None
    read_err: Optional[int] = None
    read_channels: Dict[str, float] = field(default_factory=dict)
    # memory class fields
    mem_bytes: List[int] = field(default_factory=list)
    mem_probe_addr: Optional[int] = None
    mem_probe_len: Optional[int] = None
    memory_size_bytes: Optional[int] = None
    memory_page_bytes: Optional[int] = None
    # display class fields
    display_frame_len: Optional[int] = None
    display_frame_err: Optional[int] = None
    display_status_err: Optional[int] = None
    display_status: Optional[int] = None
    # rtc class fields
    rtc_get_err: Optional[int] = None
    rtc_set_err: Optional[int] = None
    rtc_time: Dict[str, int] = field(default_factory=dict)
    output_lines: List[str] = field(default_factory=list)
    trace_path: Optional[Path] = None
    duration_s: float = 0.0


def find_renode() -> Optional[str]:
    _candidates: list[str] = []
    if os.name == "nt":
        _candidates += [
            r"C:\Program Files\Renode\bin\Renode.exe",
            r"C:\Program Files (x86)\Renode\bin\Renode.exe",
        ]
    else:
        # Common Linux install paths plus portable installs in $HOME.
        import glob
        home = os.path.expanduser("~")
        _candidates += [
            "/opt/renode/renode",
            "/usr/local/bin/renode",
            "/usr/bin/renode",
        ]
        _candidates += sorted(glob.glob(f"{home}/renode_*_portable/renode"))
    for p in _candidates:
        if Path(p).exists():
            return p
    return shutil.which("renode")


def pick_platform_repl(device: str) -> Path:
    _ = device  # Keep the public signature; platform selection is bus-level.
    hw = PLATFORMS_DIR / "stm32f103_hw_i2c.repl"
    if hw.exists():
        return hw
    raise FileNotFoundError(
        f"no generic I2C platform .repl found: {hw.name}"
    )


def synthesize_resc(device: str, firmware: Path, out_path: Path, sleep_s: int = 20) -> None:
    platform = project_relative_path(pick_platform_repl(device))
    elf = project_relative_path(firmware)
    name = f"{device}_eval"
    out_path.write_text(
        f":name: {name}\n"
        f":description: Driver evaluation run for {device}\n\n"
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


def run_self_running_firmware(
    device: str,
    *,
    firmware: Optional[Path] = None,
    resc: Optional[Path] = None,
    timeout: int = 120,
    trace_path: Optional[Path] = None,
    trace_env_var: str = "DRIVERGEN_I2C_TRACE_PATH",
) -> RenodeRunOutcome:
    """Run a self-running firmware in Renode and capture output + optional bus trace."""
    out = RenodeRunOutcome()

    renode = find_renode()
    out.renode_path = renode
    if renode is None:
        out.error = "Renode not found in PATH or common install locations"
        return out

    tmp_resc: Optional[Path] = None
    if resc is not None:
        if not resc.exists():
            out.error = f"resc file not found: {resc}"
            return out
        resc_to_run = resc
    else:
        if firmware is None or not firmware.exists():
            out.error = f"firmware not found: {firmware}"
            return out
        tmp_resc = Path(tempfile.mkstemp(suffix=".resc", prefix=f"dg_eval_{device}_")[1])
        synthesize_resc(device, firmware, tmp_resc)
        resc_to_run = tmp_resc

    if trace_path is not None and trace_path.exists():
        trace_path.unlink()

    resc_for_renode = project_relative_path(resc_to_run)

    env = dict(os.environ)
    renode_config_tmp: Optional[tempfile.TemporaryDirectory[str]] = None
    if os.name != "nt":
        # Renode uses ~/.config/renode/config.lock by default. A stale or
        # cross-process lock there makes every vector fail as no_boot before
        # the firmware starts, so isolate config state per Renode subprocess.
        renode_config_tmp = tempfile.TemporaryDirectory(prefix="dg_renode_config_")
        env["XDG_CONFIG_HOME"] = renode_config_tmp.name
    if trace_path is not None:
        env[trace_env_var] = str(trace_path)
        out.trace_path = trace_path

    cmd = [renode, "--disable-xwt", "--console", "-e", f"include @{resc_for_renode}"]
    import time as _time
    t0 = _time.time()
    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            cwd=PROJECT_ROOT,
        )
        try:
            stdout_bytes, stderr_bytes = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            # Kill entire process tree on Windows to avoid orphaned Renode.exe
            if os.name == "nt":
                subprocess.run(
                    ["taskkill", "/F", "/T", "/PID", str(proc.pid)],
                    capture_output=True, timeout=10,
                )
            else:
                proc.kill()
            proc.wait(timeout=5)
            out.timed_out = True
            out.error = f"Renode timed out after {timeout}s"
            return out
        # On normal exit, also kill the process tree to reap any orphaned
        # Mono child processes that may hold ports/handles (Windows-specific).
        if os.name == "nt":
            subprocess.run(
                ["taskkill", "/F", "/T", "/PID", str(proc.pid)],
                capture_output=True, timeout=10,
            )
        out.renode_exit = proc.returncode

        def _dec(buf) -> str:
            if buf is None:
                return ""
            if isinstance(buf, str):
                return buf
            try:
                return buf.decode("utf-8", errors="replace")
            except Exception:
                return ""

        combined = _dec(stdout_bytes) + _dec(stderr_bytes)
    except Exception as e:
        out.error = f"Renode execution error: {e}"
        return out
    finally:
        out.duration_s = _time.time() - t0
        if tmp_resc is not None:
            try:
                tmp_resc.unlink()
            except OSError:
                pass
        if renode_config_tmp is not None:
            renode_config_tmp.cleanup()

    _parse_renode_output(combined, out)
    return out


# ---------- output parser (standalone, unit-testable) ----------

_CH_RE = re.compile(r"read_ch_([^=]+)=([-+]?[\d.]+)")
_MEM_READ_RE = re.compile(r"mem_read=([0-9a-fA-F ]+)")
_INTERESTING_KEYS = (
    "DRIVER_TEST", "RESULT:", "read_raw=", "read_ch_",
    "mem_read=", "mem_probe_", "memory_size_bytes=",
    "memory_page_bytes=", "frame_len=", "output_frame_err=",
    "status_err=", "status=", "rtc_", "ERROR",
)

# RTC fields that land in RenodeRunOutcome.rtc_time[<field>].
_RTC_TIME_FIELDS = (
    "year", "month", "day", "hour", "minute", "second", "weekday",
)


def _parse_int_field(pattern: str, s: str) -> Optional[int]:
    m = re.search(pattern, s)
    if not m:
        return None
    try:
        return int(m.group(1))
    except ValueError:
        return None


def _parse_renode_output(combined: str, out: "RenodeRunOutcome") -> None:
    """Populate ``out`` from raw Renode stdout+stderr text."""
    interesting: List[str] = []
    for line in combined.splitlines():
        s = line.strip()
        if any(k in s for k in _INTERESTING_KEYS):
            interesting.append(s)
        if "DRIVER_TEST START" in s:
            out.boot_detected = True
        if "DRIVER_TEST DONE" in s:
            out.test_done = True
        if "RESULT: PASS" in s:
            out.result_pass = True
        if "RESULT: ERR" in s:
            out.result_err = True

        m_err = re.search(r"read_err=([-+]?\d+)", s)
        if m_err:
            try:
                out.read_err = int(m_err.group(1))
            except ValueError:
                pass

        m = re.search(r"read_raw=([-+]?[\d.]+)", s)
        if m:
            try:
                out.read_raw = float(m.group(1))
            except ValueError:
                pass

        for mch in _CH_RE.finditer(s):
            ch_id = mch.group(1).strip()
            try:
                out.read_channels[ch_id] = float(mch.group(2))
            except ValueError:
                pass

        mm = _MEM_READ_RE.search(s)
        if mm:
            parts = mm.group(1).split()
            parsed: List[int] = []
            ok = True
            for p in parts:
                try:
                    parsed.append(int(p, 16))
                except ValueError:
                    ok = False
                    break
            if ok:
                out.mem_bytes = parsed

        v = _parse_int_field(r"mem_probe_addr=([-+]?\d+)", s)
        if v is not None:
            out.mem_probe_addr = v
        v = _parse_int_field(r"mem_probe_len=([-+]?\d+)", s)
        if v is not None:
            out.mem_probe_len = v
        v = _parse_int_field(r"memory_size_bytes=([-+]?\d+)", s)
        if v is not None:
            out.memory_size_bytes = v
        v = _parse_int_field(r"memory_page_bytes=([-+]?\d+)", s)
        if v is not None:
            out.memory_page_bytes = v

        v = _parse_int_field(r"frame_len=([-+]?\d+)", s)
        if v is not None:
            out.display_frame_len = v
        v = _parse_int_field(r"output_frame_err=([-+]?\d+)", s)
        if v is not None:
            out.display_frame_err = v
        # Keep status lines narrowly matched to avoid capturing
        # arbitrary status strings in Renode's own log.
        v = _parse_int_field(r"\bstatus_err=([-+]?\d+)", s)
        if v is not None:
            out.display_status_err = v
        v = _parse_int_field(r"(?<!_)\bstatus=([-+]?\d+)", s)
        if v is not None:
            out.display_status = v

        v = _parse_int_field(r"rtc_get_err=([-+]?\d+)", s)
        if v is not None:
            out.rtc_get_err = v
        v = _parse_int_field(r"rtc_set_err=([-+]?\d+)", s)
        if v is not None:
            out.rtc_set_err = v
        for f in _RTC_TIME_FIELDS:
            v = _parse_int_field(rf"rtc_{f}=([-+]?\d+)", s)
            if v is not None:
                out.rtc_time[f] = v

        if "ERROR" in s or "FAIL" in s.upper():
            out.has_failures = True
    out.output_lines = interesting
