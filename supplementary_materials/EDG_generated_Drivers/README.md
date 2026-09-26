# EDG generated drivers and intermediate artifacts

This collection contains **325 runs covering 25 devices and 13 RTOSes**. The [standard specifications](../standards/README.md) are stored separately. See the [supplementary material](../supplementary_material.md) for the results and representation definitions.

## Directory layout

    <rtos>/<device>/                 Generated run artifacts
    index.json                      Task metadata and paths to generated/reference artifacts
    README.md

All paths in index.json are relative to this directory; reference specifications are linked under ../standards/. Each index entry identifies the task's bus, board, model, planning status, code attempts and available evaluation report.

## Generated artifacts

Each task directory contains its own Device IR and RTOS Contract, task and hardware context, planning records, and pipeline outcome. Where generation reached the corresponding stage, it also includes driver source/header files, an evaluation adapter, API/Test Plans, code attempts, repair feedback and evaluation reports.

| Material | Files or directory |
|---|---|
| Task and hardware context | task_package.json, fixed_task_context.json, board_context.json |
| Generated Device IR and RTOS Contract | device_ir.json, rtos_contract.json |
| API Plan and Test Plan | api_plan.json, test_plan.json, frozen_plan.json |
| Planning attempts and checks | plan_attempts/ |
| Driver code and adapter | C/header files in the task directory |
| Generation attempts and repair feedback | attempts/ |
| Run summary and outcome | RUN.json, loop_v2_result.json |
| Available benchmark reports | evaluation_report*.json |

All 325 tasks include their Device IR and RTOS Contract. **323 contain driver source/header files**; Zephyr–BME280 and RT-Thread–MPU6050 stopped during planning, and their planning records are included. The separate API/Test Plan files are extracted views of the retained frozen plan.
