# EDG standard specifications

This collection contains **25 Device IR reference specifications** and **325 task-specific RTOS reference contracts** used in the information-quality audit. See [Section 2 of the supplementary material](../supplementary_material.md#2-device-ir-and-rtos-contract-quality) for metrics and results, and [Section 5](../supplementary_material.md#5-device-ir-and-rtos-contract-formal-definitions-and-example) for the formal definitions.

## Contents

- [device_ir/](device_ir/): one JSON specification per device, with expected information, required-item flags, and datasheet evidence.
- [rtos_contract/](rtos_contract/): contracts organized by RTOS and device, including API bindings, usage requirements, and the required-information checklist.
- [evidence/](evidence/): cited datasheet page text, reference drivers, task requirements, API descriptions, and harness interfaces.

Evidence paths in the specifications are relative to this directory. Datasheet page numbers and source-code line numbers are retained. Explanatory text and comments are provided in English.

## Example and corresponding runs

The BH1750 example includes its [Device IR standard](device_ir/bh1750.json), [RT-Thread Contract standard](rtos_contract/rtthread/bh1750.json), and [generated run](../EDG_generated_Drivers/rtthread/bh1750/). The [325-task index](../EDG_generated_Drivers/index.json) links all generated runs to their corresponding standards.
