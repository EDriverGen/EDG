"""Glue between drivergen pipeline and the evaluation harness."""
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any, Callable, Dict, Optional

_ROOT = Path(__file__).resolve().parent.parent
if str(_ROOT) not in sys.path:
    sys.path.insert(0, str(_ROOT))

from evaluation.orchestrator import (  # noqa: E402
    EvaluationRun,
    OrchestratorOptions,
    evaluate as _evaluate,
    summarise,
)


def _locate_driver_dir(run_dir: Path) -> Optional[Path]:
    """Find a directory under run_dir that holds ``<device>.c`` / ``<device>.h``."""
    for candidate in (run_dir / "output", run_dir / "codegen", run_dir):
        if candidate.is_dir():
            c_files = list(candidate.glob("*.c"))
            if c_files:
                return candidate
    return None


def _infer_bus_kind(run_dir: Path, device_id: str) -> str:
    """Best-effort bus-kind inference from oracle meta (fallback i2c)."""
    try:
        from evaluation.oracle.loader import load_oracle
        oracle = load_oracle(device_id)
        return oracle.meta.bus_type or "i2c"
    except Exception:
        return "i2c"


def evaluate_run_dir(
    run_dir: Path,
    device_id: str,
    rtos_id: str,
    *,
    timeout_per_vector: int = 30,
) -> Dict[str, Any]:
    """Evaluate driver sources produced by drivergen."""
    driver_dir = _locate_driver_dir(run_dir)
    if driver_dir is None:
        return {
            "ok": False,
            "error": f"no driver sources found under {run_dir}",
        }

    opts = OrchestratorOptions(
        device_id          = device_id,
        rtos_id            = rtos_id,
        driver_dir         = driver_dir,
        bus_kind           = _infer_bus_kind(run_dir, device_id),
        timeout_per_vector = timeout_per_vector,
        report_out         = run_dir / "evaluation_report.json",
        task_package_path  = (
            run_dir / "task_package.json"
            if (run_dir / "task_package.json").is_file()
            else None
        ),
    )

    try:
        run: EvaluationRun = _evaluate(opts)
    except Exception as exc:  # pragma: no cover - eval infra failure
        return {"ok": False, "error": f"evaluation.orchestrator.evaluate raised: {exc}"}

    # JSON report already written by orchestrator (report_out set). Re-read for
    # the inline summary we return, so the pipeline gets the exact file layout.
    report_path = run_dir / "evaluation_report.json"
    if not report_path.exists():
        report_path.write_text(
            json.dumps(run.report.to_dict(), indent=2),
            encoding="utf-8",
        )

    summary = summarise(run)
    return {
        "ok":            True,
        "overall_claim": summary["overall_claim"],
        "levels":        summary["levels"],
        "driver_dir":    str(driver_dir),
        "combo":         run.report.combo,
    }


def make_pipeline_hook(*, timeout_per_vector: int = 30) -> Callable:
    """Return a hook compatible with ``drivergen.pipeline.run_job(evaluation_hook=...)``.

    Signature: ``hook(run_dir, device_id, rtos_id, _report) -> dict``.
    """
    def _hook(run_dir: Path, device_id: str, rtos_id: str, _report: dict) -> dict:
        return evaluate_run_dir(
            run_dir, device_id, rtos_id,
            timeout_per_vector=timeout_per_vector,
        )
    return _hook


__all__ = ["evaluate_run_dir", "make_pipeline_hook"]
