# Base Board Context

This directory stores board-level context files used by task packages that reference this layout.

A base board context captures the RTOS, board or integration target, bus instance, addressing mode, and other stable platform assumptions shared by multiple devices.

Most benchmark tasks compose context from:

- `data/fixed_context/platform_base_context`
- `data/fixed_context/connection_binding_context`
- `data/fixed_context/device_attachment_context`
- `data/fixed_context/task_packages`
