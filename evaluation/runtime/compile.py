"""evaluation.runtime.compile - link-mode driver compile to ARM ELF."""
from __future__ import annotations

import dataclasses
import re
import shutil
import subprocess
from pathlib import Path
from typing import Iterable, Optional

from evaluation.infrastructure import (
    HARNESS_DIR,
    HW_DIR,
    PROJECT_ROOT,
    SHARED_DIR,
    project_relative_path,
    stubs_for_rtos,
)


# ---------- result model ----------

@dataclasses.dataclass(frozen=True)
class CompileResult:
    """Outcome of a single link-mode compile."""

    success: bool
    elf_path: Optional[Path]
    errors: list[str]
    warnings: list[str]
    raw_output: str
    text_bytes: int = 0
    data_bytes: int = 0
    bss_bytes: int = 0
    return_code: Optional[int] = None
    command: list[str] = dataclasses.field(default_factory=list)

    @property
    def total_size(self) -> int:
        return self.text_bytes + self.data_bytes + self.bss_bytes


# ---------- bus & class configuration ----------

# (header, init_call, default_bus_name)
_BUS_CONFIG: dict[str, tuple[str, str, str]] = {
    "i2c":  ("hw_i2c.h",         "hw_i2c1_init()",        "i2c1"),
    "spi":  ("hw_spi.h",         "hw_spi1_init()",        "spi1"),
    "uart": ("hw_uart_bus.h",    "hw_uart_bus_init()",     "uart1"),
    # GPIO bus setup is provided by the per-vector pulse runner.
    "gpio": ("hw_uart.h",        "(void)0",               "PB5"),
}

# Supported bus-kind aliases, folded onto the canonical harness above.
_BUS_KIND_ALIASES: dict[str, str] = {
    "gpio_timing":     "gpio",
    "gpio_pulse":      "gpio",
    "gpio_oneshot":    "gpio",
    "gpio_pulse_width": "gpio",
}

_EVAL_CLASS_TO_HARNESS: dict[str, str] = {
    "single_channel": "test_main_single_channel.c",
    "multi_channel":  "test_main_multi_channel.c",
    "memory":         "test_main_memory.c",
    "display":        "test_main_display.c",
    "rtc":            "test_main_rtc.c",
}


def _gcc_arg_path(p: Path) -> str:
    return project_relative_path(p)


# ---------- staging ----------

def _stage_driver_sources(
    driver_dir: Path, adapter_path: Path, device_id: str, stage_dir: Path
) -> tuple[list[Path], Optional[str]]:
    """Copy driver sources + eval adapter into stage_dir."""
    stage_dir.mkdir(parents=True, exist_ok=True)
    for old in stage_dir.iterdir():
        if old.is_file():
            old.unlink()
        elif old.is_dir():
            shutil.rmtree(old)

    headers = list(driver_dir.glob(f"{device_id}*.h"))
    nested_headers = [
        h for h in driver_dir.rglob("*.h")
        if h.parent != driver_dir
    ]
    sources = [
        f for f in driver_dir.glob(f"{device_id}*.c")
        if "_sample" not in f.name           # samples are demos, not driver
        and "_eval_adapter" not in f.name    # adapter passed separately
    ]

    if not headers:
        return [], f"no driver headers matching {device_id}*.h in {driver_dir}"
    if not sources:
        return [], f"no driver sources matching {device_id}*.c in {driver_dir}"

    staged_c: list[Path] = []
    for h in headers:
        shutil.copy2(h, stage_dir / h.name)
        canon = h.name.replace("_ref", "")
        if canon != h.name:
            shutil.copy2(h, stage_dir / canon)
        # Some RTOS packages use a namespaced local include while still
        # emitting the header at the root. Mirror root device headers under
        # the device-id directory so those include paths compile without
        # changing generated driver code.
        namespaced = stage_dir / device_id / h.name
        namespaced.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(h, namespaced)
        if canon != h.name:
            shutil.copy2(h, namespaced.parent / canon)
    for h in nested_headers:
        dst = stage_dir / h.relative_to(driver_dir)
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(h, dst)
    for c in sources:
        dst = stage_dir / c.name
        shutil.copy2(c, dst)
        staged_c.append(dst)

    if not adapter_path.exists():
        return [], f"adapter file not found: {adapter_path}"
    adapter_dst = stage_dir / adapter_path.name
    shutil.copy2(adapter_path, adapter_dst)
    staged_c.append(adapter_dst)

    return staged_c, None


# ---------- output parsing ----------

# arm-none-eabi-gcc diagnostic line: <file>:<line>:<col>: error|warning|note: <msg>
_GCC_DIAG_RE = re.compile(
    r"^(?P<file>[^:\n]+):(?P<line>\d+)(?::(?P<col>\d+))?:\s*"
    r"(?P<sev>error|warning|note|fatal error):\s*(?P<msg>.*)$",
    re.MULTILINE,
)

_LD_ERROR_RE = re.compile(
    r"^(?:\S+/ld(?:\.bfd)?:|.*\bld(?:\.bfd)?:)\s*(?P<msg>.*)$", re.MULTILINE
)

# Linker continuation lines that carry the concrete failure text.
_LD_CONTINUATION_RE = re.compile(
    r"^(?P<msg>"
    r".*?\bundefined reference to\b.*|"
    r".*?\bmultiple definition of\b.*|"
    r".*?\brelocation truncated\b.*|"
    r".*?\brelocation against\b.*|"
    r".*?\bfirst defined here\b.*|"
    r".*?\bdefined in discarded section\b.*|"
    r".*?\bplt .*"
    r")$",
    re.MULTILINE,
)

# `collect2: error: ld returned N exit status` summary line
_COLLECT2_RE = re.compile(
    r"^(?P<msg>collect2:\s*(?:error|fatal error):.*)$", re.MULTILINE
)

_SIZE_LINE_RE = re.compile(
    r"^\s*(?P<text>\d+)\s+(?P<data>\d+)\s+(?P<bss>\d+)\s+\d+\s+[0-9a-f]+\s+\S+\s*$",
    re.MULTILINE | re.IGNORECASE,
)


def _parse_diagnostics(output: str) -> tuple[list[str], list[str]]:
    """Return (errors, warnings) parsed from gcc stdout+stderr."""
    errors: list[str] = []
    warnings: list[str] = []
    seen: set[str] = set()
    for m in _GCC_DIAG_RE.finditer(output):
        sev = m.group("sev")
        line = m.group(0).strip()
        if line in seen:
            continue
        seen.add(line)
        if sev in ("error", "fatal error"):
            errors.append(line)
        elif sev == "warning":
            warnings.append(line)
    for m in _LD_ERROR_RE.finditer(output):
        msg = m.group("msg").strip()
        if not msg:
            continue
        line = f"ld: {msg}"
        if line not in seen:
            seen.add(line)
            errors.append(line)
    # Capture continuation lines that are emitted without an `ld:` prefix.
    for m in _LD_CONTINUATION_RE.finditer(output):
        msg = m.group("msg").strip()
        if not msg:
            continue
        line = f"ld: {msg}"
        if line not in seen:
            seen.add(line)
            errors.append(line)
    # Capture the collect2 link-failure summary.
    for m in _COLLECT2_RE.finditer(output):
        msg = m.group("msg").strip()
        if msg and msg not in seen:
            seen.add(msg)
            errors.append(msg)
    return errors, warnings


def _parse_size(output: str) -> tuple[int, int, int]:
    """Extract (text, data, bss) bytes from arm-none-eabi-size output."""
    for m in _SIZE_LINE_RE.finditer(output):
        return int(m.group("text")), int(m.group("data")), int(m.group("bss"))
    return 0, 0, 0


# ---------- main entry ----------

def link_mode_compile(
    driver_dir: Path,
    adapter_path: Path,
    *,
    eval_class: str,
    bus_kind: str,
    rtos_id: str,
    out_dir: Path,
    device_id: str,
    bus_instance: Optional[str] = None,
    extra_includes: Optional[Iterable[Path]] = None,
    extra_sources: Optional[Iterable[Path]] = None,
    timeout: int = 60,
) -> CompileResult:
    """Compile driver + adapter + harness + RTOS stubs into a single ARM ELF."""
    if eval_class not in _EVAL_CLASS_TO_HARNESS:
        return CompileResult(
            success=False, elf_path=None,
            errors=[f"unknown eval_class: {eval_class!r}"],
            warnings=[], raw_output="",
        )
    # Resolve bus_kind aliases (e.g. "gpio_timing" -> "gpio") before consulting
    # _BUS_CONFIG so generated task packages with bus_kind variants still
    # compile against the canonical harness/stub set.
    canonical_bus_kind = _BUS_KIND_ALIASES.get(bus_kind, bus_kind)
    if canonical_bus_kind not in _BUS_CONFIG:
        return CompileResult(
            success=False, elf_path=None,
            errors=[f"unknown bus_kind: {bus_kind!r}"],
            warnings=[], raw_output="",
        )

    bus_header, bus_init_call, default_bus_name = _BUS_CONFIG[canonical_bus_kind]
    if bus_instance is None:
        bus_instance = default_bus_name

    harness_name = _EVAL_CLASS_TO_HARNESS[eval_class]
    harness_src = HARNESS_DIR / harness_name
    if not harness_src.exists():
        return CompileResult(
            success=False, elf_path=None,
            errors=[f"harness template not found: {harness_src} "
                    f"(eval_class={eval_class!r})"],
            warnings=[], raw_output="",
        )

    rtos_stubs_dir = stubs_for_rtos(rtos_id)
    if not rtos_stubs_dir.is_dir():
        return CompileResult(
            success=False, elf_path=None,
            errors=[f"rtos stubs not found: {rtos_stubs_dir}"],
            warnings=[], raw_output="",
        )

    out_dir = Path(out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    stage_dir = out_dir / "stage"

    staged_drivers, err = _stage_driver_sources(
        driver_dir, adapter_path, device_id, stage_dir
    )
    if err:
        return CompileResult(
            success=False, elf_path=None, errors=[err],
            warnings=[], raw_output="",
        )

    # Prefer bus-specific stubs (e.g. stubs_i2c.c / stubs_spi.c / stubs_uart.c)
    # so devices on different buses can share the same RTOS stubs directory
    # without pulling in duplicate symbols. Fall back to every .c if none of
    # the bus-tagged variants exist.  We key off the *canonical* bus_kind so
    # an alias like ``gpio_timing`` pulls in ``stubs_gpio.c`` instead of
    # silently fishing for a never-written ``stubs_gpio_timing.c``.
    bus_tagged = list(rtos_stubs_dir.glob(f"stubs_{canonical_bus_kind}.c"))
    if bus_tagged:
        rtos_stubs_c = bus_tagged
    else:
        rtos_stubs_c = list(rtos_stubs_dir.glob("*.c"))

    elf_out = out_dir / f"{device_id}_{rtos_id}.elf"
    if elf_out.exists():
        elf_out.unlink()

    sources: list[Path] = [
        SHARED_DIR / "startup.s",
        SHARED_DIR / "syscalls.c",
        *rtos_stubs_c,
        harness_src,
        *staged_drivers,
    ]
    if extra_sources:
        sources.extend(Path(p) for p in extra_sources)

    includes: list[Path] = [
        HARNESS_DIR,
        HW_DIR,
        rtos_stubs_dir,
        stage_dir,
    ]
    if extra_includes:
        includes.extend(Path(p) for p in extra_includes)

    # The bus configuration is materialised into a small header in the
    # staging directory rather than passed via -D macros. This avoids
    # shell quoting issues when macro values contain parentheses.
    bus_config_header = stage_dir / "bus_config.h"
    bus_config_header.write_text(
        "/* Created by evaluation/runtime/compile.py "
        "for harness bus injection. Do not edit. */\n"
        "#ifndef DRIVERGEN_BUS_CONFIG_H\n"
        "#define DRIVERGEN_BUS_CONFIG_H\n"
        f'#define DRIVERGEN_BUS_HEADER "{bus_header}"\n'
        f"#define DRIVERGEN_BUS_INIT_CALL {bus_init_call}\n"
        f'#define DRIVERGEN_BUS_NAME "{bus_instance}"\n'
        "#endif\n",
        encoding="utf-8",
    )

    gcc_args: list[str] = [
        "arm-none-eabi-gcc",
        "-mcpu=cortex-m3", "-mthumb", "-O0", "-g",
        "-Wall", "-Wextra", "-Wno-unused-function",
    ]
    for inc in includes:
        gcc_args.append("-I" + _gcc_arg_path(inc))
    # -include forces gcc to prepend the header to every translation unit,
    # so the harness's `#ifndef` fallback for DRIVERGEN_BUS_HEADER never
    # fires. This must come AFTER the -I flags so stage_dir is on the
    # search path.
    gcc_args.append("-include")
    gcc_args.append(_gcc_arg_path(bus_config_header))
    gcc_args += [
        "-nostartfiles", "-specs=nano.specs",
        "-T" + _gcc_arg_path(SHARED_DIR / "cortex-m3.ld"),
        "-o", _gcc_arg_path(elf_out),
    ]
    for src in sources:
        gcc_args.append(_gcc_arg_path(src))

    cmd = gcc_args

    try:
        # Force UTF-8 decoding for cross-platform gcc output.
        compile_proc = subprocess.run(
            cmd, capture_output=True, text=True, timeout=timeout,
            encoding="utf-8", errors="replace",
            cwd=PROJECT_ROOT,
        )
    except subprocess.TimeoutExpired:
        return CompileResult(
            success=False, elf_path=None,
            errors=[f"compile timed out after {timeout}s"],
            warnings=[], raw_output="", command=cmd,
        )
    except Exception as e:
        return CompileResult(
            success=False, elf_path=None,
            errors=[f"compile failed to launch: {e!r}"],
            warnings=[], raw_output="", command=cmd,
        )

    raw_output = (compile_proc.stdout or "") + (compile_proc.stderr or "")
    errors, warnings = _parse_diagnostics(raw_output)

    if compile_proc.returncode != 0 or not elf_out.exists():
        if not errors:
            errors.append(
                f"compile failed (exit={compile_proc.returncode}, no ELF produced)"
            )
        return CompileResult(
            success=False, elf_path=None, errors=errors, warnings=warnings,
            raw_output=raw_output, return_code=compile_proc.returncode,
            command=cmd,
        )

    text_bytes, data_bytes, bss_bytes = 0, 0, 0
    size_cmd = ["arm-none-eabi-size", _gcc_arg_path(elf_out)]
    try:
        size_proc = subprocess.run(
            size_cmd, capture_output=True, text=True, timeout=15,
            encoding="utf-8", errors="replace",
            cwd=PROJECT_ROOT,
        )
        text_bytes, data_bytes, bss_bytes = _parse_size(size_proc.stdout or "")
    except Exception:
        pass  # size info is best-effort

    return CompileResult(
        success=True, elf_path=elf_out, errors=errors, warnings=warnings,
        raw_output=raw_output, return_code=compile_proc.returncode,
        text_bytes=text_bytes, data_bytes=data_bytes, bss_bytes=bss_bytes,
        command=cmd,
    )


__all__ = ["CompileResult", "link_mode_compile"]
