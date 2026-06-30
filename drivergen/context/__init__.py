"""Task context loading plus RTOS context inventory assembly helpers."""

from .fixed import (
    DEVICE_ATTACHMENT_CONTEXT_ROOT,
    FIXED_CONTEXT_ROOT,
    PLATFORM_BASE_CONTEXT_ROOT,
    TASK_PACKAGES_ROOT,
    CONNECTION_BINDING_CONTEXT_ROOT,
    build_board_context,
    build_target_binding,
    load_connection_binding_context,
    load_device_attachment_context,
    load_platform_base_context,
    load_task_package,
    resolve_run_fixed_context,
    resolve_task_package_context,
    resolve_task_package_path,
)
from .inventory import (
    build_context_source_lookup,
    build_source_lookup,
    load_board_context,
)
