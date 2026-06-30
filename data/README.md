# EDGBench Data

This directory contains the data used by EDG and the EDGBench evaluator. The benchmark has 325 task packages covering 25 peripherals, 13 RTOS contexts, and I2C, SPI, UART, and GPIO-style interfaces.

## Layout

```text
data/
├── raw/             # Datasheet PDFs, grouped by peripheral
├── fixed_context/   # Frozen task packages and their platform/connection/device layers
├── references/      # Reference drivers and task-specific source evidence
└── rtos/            # RTOS source manifest and download guide
```

## Task Packages

`fixed_context/task_packages/` is the main benchmark entry point. Each task package records:

- the target RTOS/platform context;
- the fixed bus binding and polling mode;
- device attachment facts such as address, pins, and protocol hints;
- datasheet and reference paths used by generation and evaluation.

Use the CLI to list task IDs:

```bash
python -m drivergen list-task-packages
```

Some JSON files use `DriverGen/data/...` path prefixes. The loader resolves these paths relative to the current repository root.

## RTOS Sources

RTOS repositories are not downloaded by the pipeline. Download them into the paths recorded by `rtos/manifest.json`; the concise source list is in [rtos/README.md](./rtos/README.md).

## Notes

- `raw/` contains the datasheets used for Device IR extraction.
- `references/` contains source evidence and reference implementations used to define expected behavior.
- `evaluation/oracle/data/` contains the evaluator-side oracle metadata and stimuli.
