"""evaluation.ladder.l1_build - L1 build-valid judge."""
from __future__ import annotations

from pathlib import Path
from typing import Optional

from evaluation.models import LevelVerdict
from evaluation.runtime.compile import CompileResult


def judge(device_id: str, compile_result: CompileResult) -> LevelVerdict:
    """Produce an L1 verdict from a CompileResult."""
    elf = compile_result.elf_path
    elf_bytes = 0
    elf_exists = False
    if elf is not None:
        try:
            elf_exists = elf.exists() and elf.is_file()
            if elf_exists:
                elf_bytes = elf.stat().st_size
        except OSError:
            elf_exists = False

    passed = bool(
        compile_result.success
        and elf_exists
        and elf_bytes > 0
        and not compile_result.errors
    )

    if passed:
        detail = (
            f"ELF {elf_bytes} bytes "
            f"(text={compile_result.text_bytes}, "
            f"data={compile_result.data_bytes}, "
            f"bss={compile_result.bss_bytes})"
        )
    elif not compile_result.success:
        if compile_result.errors:
            # Join up to the first 4 error lines so the single `detail` field
            # carries enough context to diagnose most linker failures (header
            # + body + collect2 summary) without needing to re-read the raw
            # compile log. Longer error lists are still truncated.
            preview = "; ".join(compile_result.errors[:4])
            detail = f"compile failed: {preview}"
        else:
            detail = "compile failed (no diagnostics)"
    elif not elf_exists:
        detail = f"ELF missing: {elf}"
    elif elf_bytes == 0:
        detail = f"ELF zero-sized: {elf}"
    else:
        # success=True but errors list non-empty — defensive
        detail = f"compile reported success but has errors: {compile_result.errors[0]}"

    return LevelVerdict(
        device=device_id,
        level="L1",
        passed=passed,
        claim="build-valid",
        detail=detail,
        evidence={
            "elf_path":      str(elf) if elf is not None else None,
            "elf_bytes":     elf_bytes,
            "text_bytes":    compile_result.text_bytes,
            "data_bytes":    compile_result.data_bytes,
            "bss_bytes":     compile_result.bss_bytes,
            "return_code":   compile_result.return_code,
            "error_count":   len(compile_result.errors),
            "warning_count": len(compile_result.warnings),
            "first_errors":  compile_result.errors[:10],
        },
    )


__all__ = ["judge"]
