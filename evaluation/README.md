# EDGBench Evaluator

`evaluation/` implements the EDGBench correctness ladder for generated peripheral drivers.

## Levels

- L1 build-valid: compile and link the driver with the harness.
- L2 boot-valid: boot the firmware in Renode and reach the test harness.
- L3 protocol-valid: compare observed bus transactions with expected protocol behavior.
- L4 semantic-valid: compare driver-visible outputs with device semantics.
- L5 robust-valid: inject faults and check that the driver reports errors instead of silently returning invalid results.

## Layout

```text
evaluation/
├── cli.py             # Command-line entry point
├── orchestrator.py    # L1-L5 evaluation orchestration
├── ladder/            # Level-specific verdict logic
├── runtime/           # Build and Renode execution helpers
├── infrastructure/    # Harness code, platform .repl files, stubs, and simulated slaves
├── renode_tester/     # Renode Python peripheral models
└── oracle/            # Device oracle metadata, stimuli, and golden traces
```

## Requirements

- Python 3.10.20
- [Renode 1.16](https://renode.io/)
- [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) with `arm-none-eabi-gcc` on `PATH`
- Python dependencies from the repository `requirements.txt`

The evaluator runs Renode from the repository root and uses repository-relative `.repl` paths.

## Usage

Evaluate a generated driver bundle:

```bash
python -m evaluation.cli evaluate ^
  --device at24c256 ^
  --rtos freertos ^
  --driver-dir runs/<run_id> ^
  --bus-kind i2c ^
  --out runs/<run_id>/evaluation_report.json
```

For generation plus evaluation in one command:

```bash
python run_with_evaluation.py ^
  --combo freertos_stm32f103rb__at24c256__i2c1_polling ^
  --codegen ^
  --run-renode
```

Use `--skip-l3`, `--skip-l4`, or `--skip-l5` when isolating earlier build or boot issues.

## Oracle Data

`oracle/data/` stores device metadata, stimuli, physical ranges, required writes, and optional golden traces. These files define evaluator expectations; generated code does not read them during synthesis.
