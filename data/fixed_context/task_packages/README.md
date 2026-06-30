# Task Packages

This directory stores the fixed EDGBench task packages.

Each task package combines:
- one platform base context
- one connection binding context
- one device attachment context

The current benchmark contains 325 tasks across 25 peripherals and 13 RTOS contexts.

Use the repository CLI to list the package IDs:

```bash
python -m drivergen list-task-packages
```
