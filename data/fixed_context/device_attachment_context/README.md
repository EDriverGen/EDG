# Device Attachment Context

This directory stores device-specific attachment facts used together with platform and connection contexts.

A device attachment context captures the device bus type, default address rule, required or optional attachment pins, and key assumptions derived from datasheets and audited reference drivers.

This directory is typically composed with:
- `data/fixed_context/platform_base_context`
- `data/fixed_context/connection_binding_context`
- `data/fixed_context/task_packages`

`device_attachment_context` should describe only device-side facts, not the chosen RTOS board or connection binding.
