# Fixed Context

This directory groups the fixed-context data used by EDGBench tasks.

The current layout is:

- `platform_base_context`
  Stable RTOS / board / integration facts.
- `connection_binding_context`
  Fixed connection choices on top of a platform, such as `i2c1_polling` or `gpio_timing_default`.
- `device_attachment_context`
  Device-specific address and attachment facts derived from datasheets and audited reference drivers.
- `task_packages`
  Fully frozen combinations of platform + connection + device.
- `base_board_context`
  Board-level context files used by task packages that reference this layout.

For new work, prefer reading `task_packages` first, then resolving the referenced platform / connection / device context files.
