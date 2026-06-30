"""Stub compiler for generated driver code."""

from __future__ import annotations

import logging
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

logger = logging.getLogger(__name__)

# Project-level stub paths.
_PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
_STUBS_DIR = _PROJECT_ROOT / "evaluation" / "infrastructure" / "stubs"

# GCC diagnostic regex with optional Windows drive prefix.
_GCC_DIAG_RE = re.compile(
    r"^(?P<file>(?:[A-Za-z]:)?[^\s:]+):(?P<line>\d+):(?P<col>\d+):\s+"
    r"(?P<sev>fatal error|error|warning|note):\s+"
    r"(?P<msg>.+)$",
    re.MULTILINE,
)


@dataclass
class CompileError:
    """Single compiler diagnostic."""
    file: str
    line: int
    col: int
    severity: str   # "error" | "warning" | "note"
    message: str

    def format(self) -> str:
        return f"{self.file}:{self.line}:{self.col}: {self.severity}: {self.message}"


@dataclass
class StubCompileResult:
    """Result of a stub compilation attempt."""
    success: bool
    errors: list[CompileError] = field(default_factory=list)
    warnings: list[CompileError] = field(default_factory=list)
    raw_output: str = ""
    return_code: int = -1
    elf_path: Optional[Path] = None       # link mode: path to produced ELF
    compile_level: str = "syntax"          # "syntax" | "link"

    def error_summary(self, max_errors: int = 10) -> str:
        """Format errors into a string suitable for repair feedback."""
        if not self.errors:
            return ""
        lines = [f"Compilation failed with {len(self.errors)} error(s):"]
        for e in self.errors[:max_errors]:
            lines.append(f"  {e.format()}")
        if len(self.errors) > max_errors:
            lines.append(f"  ... and {len(self.errors) - max_errors} more errors")
        return "\n".join(lines)


_NATIVE_DETECT_TIMEOUT_S = 15


def _detect_gcc() -> str:
    """Detect arm-none-eabi-gcc availability."""
    try:
        r = subprocess.run(
            ["arm-none-eabi-gcc", "--version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=_NATIVE_DETECT_TIMEOUT_S,
        )
        if r.returncode == 0:
            return "arm-none-eabi-gcc"
    except FileNotFoundError:
        pass

    raise RuntimeError("arm-none-eabi-gcc not found in PATH.")


def _parse_diagnostics(output: str) -> tuple[list[CompileError], list[CompileError]]:
    """Parse GCC output into (errors, warnings)."""
    errors, warnings = [], []
    for m in _GCC_DIAG_RE.finditer(output):
        sev = m.group("sev")
        if sev == "fatal error":
            sev = "error"
        ce = CompileError(
            file=m.group("file"),
            line=int(m.group("line")),
            col=int(m.group("col")),
            severity=sev,
            message=m.group("msg"),
        )
        if ce.severity == "error":
            errors.append(ce)
        elif ce.severity == "warning":
            warnings.append(ce)
    return errors, warnings


# Shared link-time assets.
_SHARED_DIR = _PROJECT_ROOT / "evaluation" / "infrastructure" / "shared"
# Low-level hardware stub headers used by link mode.
_HW_DIR = _PROJECT_ROOT / "evaluation" / "infrastructure" / "hw"


def available_stub_headers(rtos_id: str) -> set[str]:
    """Return header names that the stub compiler can actually include."""
    stub_dir = _STUBS_DIR / str(rtos_id or "")
    if not stub_dir.is_dir():
        return set()
    headers: set[str] = set()
    for path in stub_dir.rglob("*.h"):
        if not path.is_file():
            continue
        rel = path.relative_to(stub_dir).as_posix()
        headers.add(rel)
    return headers


def stub_compile(
    header_text: str,
    source_text: str,
    sample_text: str,
    device_id: str,
    rtos_id: str = "rtthread",
    *,
    compile_level: str = "syntax",
    timeout: int = 30,
) -> StubCompileResult:
    """Compile generated driver code against RTOS API stubs."""
    stub_dir = _STUBS_DIR / rtos_id
    if not stub_dir.is_dir():
        return StubCompileResult(
            success=False,
            errors=[CompileError("", 0, 0, "error",
                                 f"Stub directory not found: {stub_dir}")],
        )

    dev = device_id.lower()

    try:
        gcc_cmd = _detect_gcc()
    except RuntimeError as e:
        return StubCompileResult(
            success=False,
            errors=[CompileError("", 0, 0, "error", str(e))],
        )

    # Create temp work directory
    work = Path(tempfile.mkdtemp(prefix=f"stubcc_{dev}_"))
    try:
        return _compile_in_dir(
            work, stub_dir, header_text, source_text, sample_text,
            dev, gcc_cmd, timeout, compile_level,
        )
    finally:
        shutil.rmtree(work, ignore_errors=True)


def _compile_in_dir(
    work: Path,
    stub_dir: Path,
    header_text: str,
    source_text: str,
    sample_text: str,
    dev: str,
    gcc_cmd: str,
    timeout: int,
    compile_level: str = "syntax",
) -> StubCompileResult:
    """Internal: set up files and run gcc."""

    # Write driver files
    drv_dir = work / "drv"
    drv_dir.mkdir()
    (drv_dir / f"{dev}.h").write_text(header_text, encoding="utf-8")
    (drv_dir / f"{dev}.c").write_text(source_text, encoding="utf-8")
    if sample_text.strip():
        (drv_dir / f"{dev}_sample.c").write_text(sample_text, encoding="utf-8")

    # Copy stub files (preserving subdirectory structure for path-based includes)
    stubs_work = work / "stubs"
    shutil.copytree(stub_dir, stubs_work)

    # Build source list — driver sources
    driver_c_files = [drv_dir / f"{dev}.c"]
    if (drv_dir / f"{dev}_sample.c").exists():
        driver_c_files.append(drv_dir / f"{dev}_sample.c")

    if compile_level == "link":
        # Link all stub source layouts into one ELF.
        shared_dir = _SHARED_DIR
        elf_out = work / "test.elf"
        stub_c_files = sorted(stubs_work.glob("stubs*.c"))
        if not stub_c_files:
            return StubCompileResult(
                success=False,
                errors=[CompileError(str(stubs_work), 0, 0, "error",
                                     f"no stubs*.c found in {stubs_work}")],
                compile_level="link",
            )
        gcc_args = [
            gcc_cmd,
            "-mcpu=cortex-m3", "-mthumb",
            "-O0", "-g",
            "-Wall", "-Wextra",
            f"-I{stubs_work}",
            f"-I{drv_dir}",
            f"-I{_HW_DIR}",
            "-nostartfiles",
            "-specs=nano.specs",
            "-Wl,--allow-multiple-definition",
            f"-T{shared_dir / 'cortex-m3.ld'}",
            "-o", str(elf_out),
            str(shared_dir / "startup.s"),
            str(shared_dir / "syscalls.c"),
            *(str(p) for p in stub_c_files),
        ] + [str(f) for f in driver_c_files]
    else:
        # Syntax-only mode.
        elf_out = None
        gcc_args = [
            gcc_cmd,
            "-mcpu=cortex-m3", "-mthumb",
            "-O0", "-g",
            "-Wall", "-Wextra",
            "-fsyntax-only",
            f"-I{stubs_work}",
            f"-I{drv_dir}",
        ] + [str(f) for f in driver_c_files]

    cmd = gcc_args

    logger.info("Stub compile: %s", " ".join(cmd[-5:]))  # last 5 args for brevity

    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return StubCompileResult(
            success=False,
            errors=[CompileError("", 0, 0, "error",
                                 f"Compilation timed out after {timeout}s")],
        )

    raw = (proc.stdout or b"") + (proc.stderr or b"")
    raw = raw.decode("utf-8", errors="replace")
    errors, warnings = _parse_diagnostics(raw)

    # For link mode, check if ELF was produced
    elf_produced = elf_out and elf_out.exists() if elf_out else None

    result = StubCompileResult(
        success=(proc.returncode == 0 and len(errors) == 0),
        errors=errors,
        warnings=warnings,
        raw_output=raw,
        return_code=proc.returncode,
        elf_path=elf_out if elf_produced else None,
        compile_level=compile_level,
    )

    if result.success:
        logger.info("Stub compilation OK (%d warnings)", len(warnings))
    else:
        logger.warning("Stub compilation FAILED (%d errors, %d warnings)",
                       len(errors), len(warnings))

    return result
