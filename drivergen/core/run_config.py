from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .catalog import DATA_ROOT, PROJECT_ROOT

SUPPORTED_PIPELINE_NAME = "DriverGen"


@dataclass(frozen=True)
class PipelineRunConfig:
    """Resolved pipeline input."""

    run_id: str
    device_id: str
    pdf_path: Path
    rtos_id: str
    board_context_path: Path | None
    task_package_path: Path | None
    provider_name: str
    provider_model: str
    pipeline_name: str = SUPPORTED_PIPELINE_NAME
    source_ref: str | None = None

    def to_dict(self) -> dict[str, Any]:
        return {
            "run_id": self.run_id,
            "device": {
                "device_id": self.device_id,
                "pdf_path": str(self.pdf_path),
            },
            "target": {
                "rtos_id": self.rtos_id,
                "board_context_path": str(self.board_context_path) if self.board_context_path else None,
                "task_package_path": str(self.task_package_path) if self.task_package_path else None,
            },
            "provider": {
                "name": self.provider_name,
                "model": self.provider_model,
            },
            "pipeline": {
                "name": self.pipeline_name,
            },
            "source_ref": self.source_ref,
        }


def _resolve_input_path(raw_path: str | Path, base_path: Path | None = None) -> Path:
    candidate = Path(raw_path)
    if candidate.is_absolute():
        return candidate

    if base_path is not None:
        base_relative = (base_path.parent / candidate).resolve()
        if base_relative.exists():
            return base_relative

    project_relative = (PROJECT_ROOT / candidate).resolve()
    if project_relative.exists():
        return project_relative

    return project_relative


def find_device_pdf(device_id: str) -> Path:
    root = DATA_ROOT / "raw" / device_id
    candidates = sorted(root.glob("*.pdf"))
    if not candidates:
        raise FileNotFoundError(f"No PDF found for device {device_id}: {root}")
    return candidates[0]


def run_config_from_task_package(
    task_package_ref: str | Path,
    *,
    provider: str,
    model: str,
    pipeline_name: str = SUPPORTED_PIPELINE_NAME,
) -> PipelineRunConfig:
    from ..context.fixed import resolve_task_package_context

    context = resolve_task_package_context(task_package_ref)
    task_package = context["task_package"]
    fixed = task_package["fixed_task_context"]
    device_id = str(fixed["device"]["device_id"])
    rtos_id = str(fixed["platform"]["rtos"])
    package_id = str(task_package["package_id"])
    task_package_path = Path(context["task_package_path"]).resolve()
    return PipelineRunConfig(
        run_id=f"{package_id}__{provider}_{model}",
        device_id=device_id,
        pdf_path=find_device_pdf(device_id),
        rtos_id=rtos_id,
        board_context_path=None,
        task_package_path=task_package_path,
        provider_name=provider,
        provider_model=model,
        pipeline_name=pipeline_name,
        source_ref=package_id,
    )


def load_run_config(path: Path) -> PipelineRunConfig:
    resolved = Path(path).resolve()
    payload = json.loads(resolved.read_text(encoding="utf-8"))
    device = payload["device"]
    target = payload["target"]
    provider = payload["provider"]
    pipeline = payload.get("pipeline") or {}
    config = PipelineRunConfig(
        run_id=str(payload["run_id"]),
        device_id=str(device["device_id"]),
        pdf_path=_resolve_input_path(device["pdf_path"], resolved),
        rtos_id=str(target["rtos_id"]),
        board_context_path=(
            _resolve_input_path(target["board_context_path"], resolved)
            if target.get("board_context_path")
            else None
        ),
        task_package_path=(
            _resolve_input_path(target["task_package_path"], resolved)
            if target.get("task_package_path")
            else None
        ),
        provider_name=str(provider["name"]),
        provider_model=str(provider["model"]),
        pipeline_name=str(pipeline.get("name") or SUPPORTED_PIPELINE_NAME),
        source_ref=payload.get("source_ref"),
    )
    if config.pipeline_name != SUPPORTED_PIPELINE_NAME:
        raise ValueError(
            f"Unsupported pipeline '{config.pipeline_name}'. Supported pipeline: {SUPPORTED_PIPELINE_NAME}"
        )
    if not config.pdf_path.exists():
        raise FileNotFoundError(f"PDF not found: {config.pdf_path}")
    if not config.board_context_path and not config.task_package_path:
        raise ValueError("Run config target must provide either board_context_path or task_package_path.")
    if config.board_context_path and not config.board_context_path.exists():
        raise FileNotFoundError(f"Board context file not found: {config.board_context_path}")
    if config.task_package_path and not config.task_package_path.exists():
        raise FileNotFoundError(f"Task package file not found: {config.task_package_path}")
    return config
