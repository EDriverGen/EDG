"""Top-level orchestration and CLI entrypoints."""

from .cli import build_parser, main
from .orchestrator import (
    _board_run_token,
    _run_config_run_name,
    _timestamp,
    extract_device_ir_structured,
    run_pipeline,
    run_task_package,
)
