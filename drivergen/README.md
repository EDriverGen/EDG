# DriverGen

`drivergen/` implements the EDG generation framework. It turns datasheets, fixed hardware context, and local RTOS repositories into generated driver code and evaluation adapters.

## Workflow

1. Parse datasheet PDFs with Docling and extract structured document blocks.
2. Filter relevant evidence and build a checked Device IR.
3. Resolve local RTOS sources and derive an RTOS Contract for the task.
4. Build expected transactions and output semantics from the Device IR.
5. Generate driver code, generate an evaluation adapter, and repair with validation feedback.

## Layout

```text
drivergen/
├── datasheet/   # Docling-backed PDF parsing and relevance filtering
├── core/        # Shared schemas, models, validators, and run config
├── rtos/        # RTOS source discovery, slot grounding, and contract building
├── codegen/     # Driver synthesis, adapters, checks, and repair loop
├── llm/         # Provider wrappers and prompts
├── context/     # Fixed-context and task-package loaders
└── pipeline/    # CLI and orchestration
```

## CLI

List RTOS profiles and task packages:

```bash
python -m drivergen list-rtos
python -m drivergen list-task-packages
```

Run one fixed task package:

```bash
python -m drivergen run --combo freertos_stm32f103rb__at24c256__i2c1_polling --codegen --no-renode
```

Extract a Device IR from one datasheet:

```bash
python -m drivergen extract-structured --pdf data/raw/lm75a/LM75A.pdf --device-id lm75a --bus i2c
```

`run_demo.py` is a convenience wrapper around this CLI.

## LLM Providers

Supported provider IDs are:

- `openai`
- `aliyun`
- `deepseek`

Common environment variables:

```bash
set OPENAI_API_KEY=...
set DASHSCOPE_API_KEY=...
set DEEPSEEK_API_KEY=...
```

Use `--provider` and `--model` on generation commands to select the backend.

## Outputs

Runs are written under `runs/` unless `--output-dir` is provided. A typical code-generation run includes:

- `run_config.json`
- `structured_document.json`
- `device_ir.json`
- `rtos_contract.json`
- `expected_transactions.json`
- generated `<device>.c`, `<device>.h`, and `<device>_eval_adapter.c`
- `validation_report.json`

## Dependencies

Use Python 3.10.20 and install repository-level Python dependencies from `requirements.txt`. For full code generation with runtime probing, also install Renode and `arm-none-eabi-gcc`; details are in [../evaluation/README.md](../evaluation/README.md). If Docling GPU execution is needed, install a PyTorch build that matches the target CUDA runtime.
