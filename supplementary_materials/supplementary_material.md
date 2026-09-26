# EDG: Supplementary Results and Representation Definitions

## Overview

This supplement presents the driver-availability survey, Device IR and RTOS Contract quality results, stage-level token usage, reasoning-effort comparison, and a 30-task DMA generation experiment with a polling comparison. It also defines the two intermediate representations and illustrates their use with a BH1750 driver on RT-Thread.

The accompanying artifacts are organized into three collections:

- [Generated drivers and run records](EDG_generated_Drivers/README.md): 325 tasks, including each task's intermediate artifacts, available driver code, and planning/repair records. The [task index](EDG_generated_Drivers/index.json) links to individual runs and their reference specifications.
- [Standard specifications and source evidence](standards/README.md): [25 Device IR standards](standards/device_ir/), [325 RTOS Contract standards](standards/rtos_contract/), and the [supporting datasheet/API material](standards/evidence/).
- [DMA task packages](EDG_DMA_Tasks/README.md): 30 tasks with their platform, connection, and device contexts, Device IRs, device-oracle inputs, and DMA generation/validation requirements. The [DMA task index](EDG_DMA_Tasks/index.json) lists all selected pairs.

## Contents

1. [Driver availability and platform dependencies](#1-driver-availability-and-platform-dependencies)
2. [Device IR and RTOS Contract quality](#2-device-ir-and-rtos-contract-quality)
3. [Stage-level token usage](#3-stage-level-token-usage)
4. [Reasoning-effort comparison](#4-reasoning-effort-comparison)
5. [Device IR and RTOS Contract: formal definitions and example](#5-device-ir-and-rtos-contract-formal-definitions-and-example)
   - [Inputs and Device IR](#51-inputs-and-device-ir)
   - [RTOS Contract and template expansion](#52-rtos-contract-and-template-expansion)
   - [Worked example: BH1750 on RT-Thread](#53-worked-example-bh1750-on-rt-thread)
6. [DMA driver generation on 30 tasks](#6-dma-driver-generation-on-30-tasks)
   - [DMA and its place in the driver stack](#61-dma-and-its-place-in-the-driver-stack)
   - [How DMA requirements enter EDG](#62-how-dma-requirements-enter-edg)
   - [Subset selection and hardware scope](#63-subset-selection-and-hardware-scope)
   - [Aggregate results and polling comparison](#64-aggregate-results-and-polling-comparison)
   - [Interpretation](#65-interpretation)

## 1. Driver availability and platform dependencies

The survey covers **325 device–RTOS pairs (25 devices × 13 RTOSes)**. A positive entry requires implementation source for the exact device, using any supported bus. Native drivers belong to the RTOS kernel or its own driver tree; third-party implementations include companion BSP/SDK code.

### Table 1. Overall driver availability

| Source | Pairs | Coverage |
| --- | --- | --- |
| Native | 36 | 11.1% |
| Third-party/BSP/SDK | 37 | 11.4% |
| Combined, deduplicated | 71 | 21.8% |
| No qualifying implementation identified | 254 | 78.2% |

### Table 2. Driver availability by RTOS

| RTOS | Native / 25 | Third-party / 25 | Union / 25 |
| --- | --- | --- | --- |
| FreeRTOS | 0 | 2 | 2 |
| CMSIS-RTX | 0 | 1 | 1 |
| ThreadX | 0 | 1 | 1 |
| TobudOS | 0 | 2 | 2 |
| RTEMS | 1 | 1 | 2 |
| Zephyr | 14 | 2 | 15 |
| NuttX | 9 | 0 | 9 |
| RT-Thread | 0 | 11 | 11 |
| Apache Mynewt | 5 | 2 | 7 |
| RIOT | 6 | 0 | 6 |
| ChibiOS | 1 | 9 | 9 |
| XiUOS | 0 | 2 | 2 |
| OpenHarmony LiteOS-M | 0 | 4 | 4 |
| Total / 325 | 36 | 37 | 71 |

The two overlapping pairs are Zephyr–SSD1306 and ChibiOS–LSM303DLHC. Seven RTOSes have no qualifying native driver for the selected devices. Zephyr and NuttX account for **23/36 native pairs (63.9%)**. Combined coverage is highest for BH1750 (8/13 RTOSes) and MPU6050 (7/13); no qualifying implementation was identified for AT24C256, EMC1413, MCP23017, TMP105, TMP421, or W25Q64JV under the exact-device rule.

### Table 3. Platform dependencies of inspected implementations

| Category | Native / 36 | Third-party / 37 | Share of third-party entries |
| --- | --- | --- | --- |
| RTOS interfaces or board callbacks | 36 | 15 | 40.5% |
| Fixed instances or vendor-header dependencies | 0 | 5 | 13.5% |
| Platform-specific APIs, controller setup, or timing | 0 | 17 | 45.9% |

Of the third-party implementations, **17/37 (45.9%)** have hardware-platform dependencies, including ESP-IDF interfaces, Hi3861 pin setup, STM32 HAL handles, or CPU-specific delays. EDG uses the target hardware context to match device protocols to RTOS/hardware interfaces and generate code, reducing manual binding and adaptation work.

Coverage is concentrated in a small part of the device–RTOS matrix, suggesting that familiar devices can still require substantial integration work across platforms. All 36 native implementations use RTOS interfaces or board callbacks to isolate hardware differences. This supports an integration strategy that combines reusable device-protocol knowledge with bindings for the target platform. EDG automates this matching and code construction, addressing both uncovered pairs and the adaptation work associated with platform-dependent implementations.

## 2. Device IR and RTOS Contract quality

The audit covers **25 Device IRs** against 26 specification documents and **325 generated RTOS Contracts** against task-specific reference/API materials. Judgments use semantic information items: correct (C), wrong (W), and missing (M). Equivalent wording and correct textual formulas are accepted; duplicate facts are counted once.

The audit references are available as [Device IR standards](standards/device_ir/) and [RTOS Contract standards](standards/rtos_contract/). The [task index](EDG_generated_Drivers/index.json) links each generated run to its corresponding standards.

Accuracy is C/(C+W). Correct coverage measures required information that is present and correct; omission measures required information that is missing. Unconfirmed required items are counted as omissions. Out-of-scope items are excluded. Coverage and omission use the same required-item denominator.

### Table 4. Aggregate information-quality metrics

| Representation | Artifacts | Accuracy | Correct coverage | Omission |
| --- | --- | --- | --- | --- |
| Device IR | 25 | 96.97% (703/725) | 83.16% (489/588) | 14.80% (87/588) |
| RTOS Contract | 325 | 97.50% (1,092/1,120) | 77.83% (1,092/1,403) | 20.17% (283/1,403) |

Device IR records 703 correct, 22 wrong, 92 items counted as missing, and 7 out-of-scope items; its required subset contains **489 correct, 12 wrong, and 87 missing items**. RTOS Contract records **1,092 correct, 28 wrong, and 283 items counted as missing**. Wrong required items account for the gap between correct coverage plus omission and 100%: **2.04% for Device IR and 2.00% for RTOS Contract**.

At both stages, missing required information substantially outnumbers incorrect required information. The main improvement opportunity is therefore to increase completeness while maintaining the observed accuracy. The structured representations make this actionable: each uncovered requirement can be associated with a specific operation, platform capability, or source-evidence request.

### Table 5. Quality by device (%)

Each Device IR is audited once; each device's Contract results aggregate its 13 RTOS tasks. A = accuracy, Cvg = correct coverage, O = omission.

| Device | IR A | IR Cvg | IR O | Contract A | Contract Cvg | Contract O |
| --- | --- | --- | --- | --- | --- | --- |
| ADXL345 | 100.00 | 90.48 | 9.52 | 97.22 | 61.40 | 36.84 |
| AT24C256 | 100.00 | 85.00 | 15.00 | 100.00 | 84.75 | 15.25 |
| BH1750 | 100.00 | 91.43 | 8.57 | 98.00 | 83.05 | 15.25 |
| BME280 | 100.00 | 70.00 | 30.00 | 98.00 | 83.05 | 15.25 |
| DHT22 | 68.42 | 54.17 | 20.83 | 94.00 | 68.12 | 27.54 |
| DPS310 | 96.97 | 62.96 | 37.04 | 98.00 | 83.05 | 15.25 |
| DS18B20 | 100.00 | 93.10 | 6.90 | 95.92 | 68.12 | 28.99 |
| DS3231 | 97.87 | 85.71 | 10.71 | 100.00 | 91.67 | 8.33 |
| EMC1413 | 93.10 | 77.78 | 16.67 | 97.83 | 90.00 | 8.00 |
| HC-SR04 | 100.00 | 91.67 | 8.33 | 97.92 | 71.21 | 27.27 |
| LM75A | 88.57 | 94.74 | 0.00 | 97.83 | 84.91 | 13.21 |
| LSM303DLHC | 100.00 | 80.00 | 20.00 | 95.74 | 84.91 | 11.32 |
| MAX31855 | 100.00 | 94.12 | 5.88 | 97.50 | 66.10 | 32.20 |
| MCP23017 | 100.00 | 76.47 | 23.53 | 95.56 | 86.00 | 10.00 |
| MCP3008 | 87.50 | 87.50 | 6.25 | 94.59 | 63.64 | 32.73 |
| MH-Z19B | 100.00 | 85.71 | 14.29 | 90.00 | 47.37 | 47.37 |
| MPU6050 | 100.00 | 85.71 | 14.29 | 95.56 | 86.00 | 10.00 |
| PCA9685 | 100.00 | 85.71 | 14.29 | 100.00 | 87.04 | 12.96 |
| PCF8574 | 100.00 | 89.47 | 10.53 | 100.00 | 89.80 | 10.20 |
| SHT30 | 100.00 | 96.00 | 4.00 | 98.00 | 81.67 | 16.67 |
| SSD1306 | 97.37 | 85.19 | 11.11 | 97.73 | 86.00 | 12.00 |
| TMP105 | 100.00 | 82.14 | 17.86 | 100.00 | 86.54 | 13.46 |
| TMP421 | 96.43 | 89.29 | 10.71 | 97.78 | 84.62 | 13.46 |
| VL53L0X | 96.97 | 72.73 | 22.73 | 100.00 | 85.00 | 15.00 |
| W25Q64JV | 91.89 | 80.77 | 19.23 | 96.77 | 55.56 | 42.59 |
| Micro average | 96.97 | 83.16 | 14.80 | 97.50 | 77.83 | 20.17 |

For Device IR, completeness is the main remaining issue: status/error behavior, encoding/conversion, and timing account for **54/87 required omissions (62.1%)**. BME280 illustrates accurate but incomplete extraction (100% accuracy, 70% coverage); DHT22's lower accuracy also calls for better source interpretation and document-version alignment.

A plausible explanation is that these requirements are dispersed across register descriptions, timing diagrams, formulas, and mode-specific notes. Future extraction could organize completeness checks around each device operation, tracing its preconditions, waits, data conversion, and error handling. This would guide the document-analysis agent toward missing dependencies and enable targeted additions to an existing IR.

### Table 6. RTOS Contract quality by RTOS

Each row covers 25 devices. Required items include C, W, and M.

| RTOS | Required items | Accuracy | Correct coverage | Omission |
| --- | --- | --- | --- | --- |
| FreeRTOS | 68 | 95.16% | 86.76% | 8.82% |
| CMSIS-RTX | 138 | 99.24% | 94.20% | 5.07% |
| ThreadX | 146 | 100.00% | 56.16% | 43.84% |
| TobudOS | 66 | 100.00% | 86.36% | 13.64% |
| RTEMS | 89 | 100.00% | 77.53% | 22.47% |
| Zephyr | 90 | 98.67% | 82.22% | 16.67% |
| NuttX | 124 | 100.00% | 90.32% | 9.68% |
| RT-Thread | 102 | 87.64% | 76.47% | 12.75% |
| Apache Mynewt | 118 | 87.06% | 62.71% | 27.97% |
| RIOT | 109 | 98.80% | 75.23% | 23.85% |
| ChibiOS | 119 | 100.00% | 68.07% | 31.93% |
| XiUOS | 145 | 100.00% | 77.24% | 22.76% |
| OpenHarmony LiteOS-M | 89 | 100.00% | 92.13% | 7.87% |
| Micro average | 1403 | 97.50% | 77.83% | 20.17% |

Contract coverage ranges from **56.16% for ThreadX to 94.20% for CMSIS-RTX**. ThreadX and ChibiOS account for **102/283 omissions (36.0%)**. The main completion needs concern initialization details, parameter structures, wrapper/callback bindings, and evidence for equivalent API usage. Required-item checklists, implementation tracing, and bounded requests to upstream agents are the corresponding improvement directions.

The device-level results also help locate gaps: MH-Z19B has 100% Device IR accuracy and 85.71% IR coverage, while its Contract coverage is 47.37%. One possible explanation for platform-side omissions is that usable bindings span several layers of initialization, wrappers, and board support. Tracing these dependencies and requesting specific missing bindings could improve completeness; explicit request budgets and stopping rules would keep such cross-stage completion predictable.

## 3. Stage-level token usage

Means per task over the **complete 325-task run**. Cost uses the study's rates of **$0.14/M input tokens (cache-miss)** and **$0.28/M output tokens**.

### Table 7. Mean stage tokens and cost

| Stage | Input | Output | Total tokens | Token share | Cost (USD) |
| --- | --- | --- | --- | --- | --- |
| Device IR | 6,484 | 1,849 | 8,333 | 8.41% | $0.00143 |
| RTOS Contract | 46,132 | 7,079 | 53,211 | 53.73% | $0.00844 |
| Synthesis | 19,489 | 2,802 | 22,291 | 22.51% | $0.00351 |
| Repair | 13,486 | 1,710 | 15,196 | 15.34% | $0.00237 |
| Total | 85,591 | 13,440 | 99,031 | 100.00% | $0.01575 |

The mean cost is **approximately $0.0157 per task**. RTOS Contract consumes **53.73% of total tokens**, making focused retrieval and reusable platform context useful optimization targets.

Platform analysis consumes more tokens than synthesis and repair combined. A likely contributor is the need to inspect declarations, implementations, and usage contexts across the RTOS. Together with the completeness audit, this identifies platform analysis as a priority for improving both quality and efficiency. Future versions could reuse verified bindings for the same RTOS version and board configuration, then retrieve additional context only for device-specific requirements.

## 4. Reasoning-effort comparison

Results on the **103-task subset**, using the same DeepSeek model, task inputs, pipeline, and repair bound. Token, time, and cost values are per-task means; costs use the rates in Table 7.

### Table 8. Results by reasoning effort

| Effort | L5 pass rate | Input tokens | Output tokens | Of which reasoning | Total tokens | Time | Cost (USD) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Off | 49.51% | 86,632 | 13,881 | 0 | 100,513 | 219 s | $0.01602 |
| Low | 53.40% | 87,847 | 24,869 | 9,582 | 112,716 | 316 s | $0.01926 |
| High | 55.34% | 88,996 | 37,432 | 19,360 | 126,428 | 538 s | $0.02294 |
| Max | 58.25% | 91,147 | 58,582 | 37,501 | 149,729 | 965 s | $0.02916 |

Output tokens include reasoning tokens. Max improves L5 by **8.74 percentage points**, using **1.49× total tokens**, **1.82× cost**, and **4.41× runtime** relative to off. The main configuration disables thinking for speed, resource use, and result reuse; backbone comparisons also avoid differences in how models implement effort levels.

Low captures about **45% of the observed Off-to-Max L5 improvement**, with approximately **20% higher cost** and **44% longer runtime** than Off. This makes Low an economical setting on this subset, while Max provides the highest observed pass rate. Additional reasoning may help reconcile device requirements with platform bindings and interpret repair feedback. A useful next step is to evaluate selective escalation for uncertain bindings or unsuccessful repairs, and identify which device–RTOS tasks benefit most.

## 5. Device IR and RTOS Contract: formal definitions and example

### 5.1 Inputs and Device IR

Let D denote the device documentation, R the target RTOS repository, and H the fixed hardware context: board, selected bus and instance, pins, device address, and connections. EDG separates device-specification analysis from platform-interface grounding, following the corresponding activities in driver development. Structured artifacts retain the necessary facts and evidence while giving later stages focused, checkable context.

For the device and selected bus mode, define:

$$
I_D=(B,G,O,T,V,F,E_D).
$$

| Component | Definition |
| --- | --- |
| B: bus and addressing | Device-side bus mode, address format, legal addresses, and transaction framing. H selects the concrete deployment. |
| G: registers and commands | Register addresses, command encodings, widths, access properties, and bitfields. |
| O: operations | Initialization, read/write, reset, and measurement steps, with ordering, conditions, and dependencies. |
| T: timing | Conversion waits, transaction intervals, pulse widths, and other constraints, including values, units, and applicability conditions. |
| V: data semantics | Byte order, signedness, effective bits, conversion formulas, physical units, and channel meanings. |
| F: status and errors | Device states, exceptional conditions, detection rules, and required responses. |
| E_D: evidence | Documentation sections, pages, excerpts, or tables supporting the represented facts. |

The components describe an operation jointly. O references the commands or registers in G; T constrains when the steps execute; V maps the returned bytes to output values; F specifies applicable state and error behavior. E_D ties these requirements to their sources. For example, a conversion operation can impose a minimum interval between completing its trigger and starting the result read, followed by a specified signedness and unit conversion.

Device IR organizes information dispersed across the manual into one device specification. Its device-side requirements can be reused when generating for another RTOS with the corresponding bus mode and hardware context.

### 5.2 RTOS Contract and template expansion

RTOS Contract starts from a **base template K₀**. The template supplies common capability categories and the structure of binding records. EDG instantiates the applicable common requirements and extends them with capabilities required by the device and deployment:

$$
S_0=\operatorname{BaseSlots}(K_0,H),\qquad
S_D=\operatorname{RequiredSlots}(I_D,H),\qquad
S=S_0\cup S_D.
$$

Each slot names a platform capability independently of the concrete RTOS function, such as `i2c.write`, `i2c.read`, `i2c.write_then_read`, or `runtime.delay_ms`. Device operations determine the required transfer directions and sequencing; timing and hardware context add the relevant wait, initialization, and binding requirements.

Define the contract and its binding map as:

$$
K_{R,H}=(S,\beta),\qquad \beta(s)=(A_s,U_s,E_s).
$$

| Binding component | Definition |
| --- | --- |
| Aₛ: APIs and dependencies | Selected symbols, signatures, headers, related types, and supporting platform dependencies. |
| Uₛ: usage conditions | Initialization, bus/device-handle binding, parameter construction, call ordering, buffer use, and return/error conventions. |
| Eₛ: source evidence | Source declarations, implementations, and usage examples supporting the binding and its conditions. |

A slot may be implemented by multiple cooperating calls. Conversely, one API can implement different slots through different parameters: a message-based I²C transfer function can support both reading and writing. H constrains the selected bus instance, address, pins, and available handles.

During construction, β may be partial. Missing bindings identify capabilities to complete. The coverage condition for a complete contract is:

$$
S\subseteq\operatorname{dom}(\beta).
$$

Completeness also requires compatible signatures and types, supported usage conditions, source evidence, and consistency with H. Construction follows **template instantiation → device/deployment requirement expansion → API retrieval and binding → completion and consistency checking**. This produces a contract for the specific device–RTOS–hardware task.

### 5.3 Worked example: BH1750 on RT-Thread

Related artifacts: [Device IR standard](standards/device_ir/bh1750.json), [RTOS Contract standard](standards/rtos_contract/rtthread/bh1750.json), and [generated run](EDG_generated_Drivers/rtthread/bh1750/).

Consider the benchmark's **STM32F103RB, RT-Thread, I²C1 polling** configuration, with BH1750 at **7-bit address 0x23**, one-shot high-resolution mode, and default measurement-time register **MTreg = 69**.

**Device IR.** B records the address rule (0x23 with ADDR low, 0x5C with ADDR high) and single-byte command interface. G includes Power On `0x01`, Reset `0x07`, and One-Time H-Resolution `0x20`. O specifies the measurement sequence; T records the maximum 180 ms conversion wait for this mode and setting. V defines the two-byte, unsigned, high-byte-first output and `lux = raw / 1.2`. F records that reset is valid after leaving power-down and that one-shot conversion returns the device to power-down. E_D retains the corresponding instruction, timing, and conversion evidence.

A measurement sequence is:

```text
Write 0x01                   Power on
Write 0x07, if resetting      Clear the data register after power-on
Write 0x20                   Start one-shot high-resolution measurement
Wait for conversion          Use the 180 ms bound for the selected setting
Read two bytes               Direct result read
raw = (high_byte << 8) | low_byte
lux = raw / 1.2
```

**RTOS Contract.** The base template supplies the binding-record structure; the task requires bus acquisition, byte writes, result reads, and a millisecond wait. The bindings make these operations concrete:

| Required capability | RT-Thread binding | Usage conditions |
| --- | --- | --- |
| Acquire the task bus | `rt_i2c_bus_device_find("i2c1")` | BSP has registered the bus; retain the returned `struct rt_i2c_bus_device *` and check for null. |
| Send a command | `rt_i2c_transfer` with one `struct rt_i2c_msg` | `addr = 0x23`, `flags = RT_I2C_WR`, `len = 1`, and `buf` points to the opcode. |
| Wait for conversion | `rt_thread_mdelay` | Argument is in milliseconds; the selected wait must satisfy T under the target timing configuration. |
| Read the measurement | `rt_i2c_transfer` with one `struct rt_i2c_msg` | `addr = 0x23`, `flags = RT_I2C_RD`, `len = 2`, and `buf` points to the output buffer. |

For the read slot, Aₛ includes the bus/message types and the target declaration:

```c
rt_ssize_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg msgs[],
                          rt_uint32_t num);
```

Uₛ specifies the message fields, valid buffer, bus handle, and success convention: for one requested message, the return value must be 1; otherwise the driver reports a transfer error. The interface is made available through the RT-Thread device headers. Eₛ records its declaration in `components/drivers/include/drivers/dev_i2c.h` and implementation in `components/drivers/i2c/dev_i2c_core.c`, together with the applicable usage evidence. The same API realizes both write and read slots through the direction flag and buffer length.

**Connection to generation and testing.** Device IR determines the command bytes, ordering, wait, and conversion. RTOS Contract determines how those operations are invoked and checked on RT-Thread. For example, bytes `0x01 0x20` decode to raw 288 and **240 lx**, or **24,000** if API Plan exposes lux multiplied by 100. Test Plan can use this value together with the expected command/read sequence and wait constraint. Together, I_D, K, and H give synthesis and test planning a shared specification for the driver.

## 6. DMA driver generation on 30 tasks

### 6.1 DMA and its place in the driver stack

Direct memory access (DMA) transfers data between memory buffers and a peripheral controller's data registers after software configures the transfer. The CPU initiates the operation and handles completion or errors, while the DMA engine performs the individual data movements. For an external SPI, I²C, or UART device, this mechanism belongs to the **MCU's transfer implementation**: the bus still carries the device's commands and data under its original protocol. DMA availability therefore depends on the selected MCU controller and the target RTOS/BSP integration. STM32F1 exposes DMA request mappings for SPI, I²C, and USART controllers. See the [STM32F1 reference manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f103xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf).

Three aspects remain distinct:

- **Device behavior:** command bytes, register addresses, measurement delays, framing, checksums, and value conversion remain device requirements.
- **Transfer implementation:** DMA adds controller/channel configuration, buffer ownership, transfer submission, completion synchronization, and error handling.
- **Application interface:** a driver can expose a blocking call that waits for a DMA transfer, or an asynchronous call that reports completion later. API synchrony and the underlying data movement mechanism are separate choices. For example, Zephyr's [SPI API](https://docs.zephyrproject.org/latest/doxygen/html/group__spi__interface.html) provides synchronous and asynchronous transfer interfaces.

DMA completion must be interpreted according to the controller/API contract before consuming a receive buffer, releasing chip select, or reusing a transmit buffer. Device-level waits also remain necessary: moving a measurement command or flash programming data does not eliminate the device's conversion or programming time. These requirements make DMA generation a test of platform binding and transfer-state management alongside the existing protocol and semantic checks.

### 6.2 How DMA requirements enter EDG

A DMA task specifies the transfer mode together with the existing hardware context. **Device IR** retains device protocol, timing, and data interpretation. **RTOS Contract** expands the required platform capabilities to include the available DMA transfer path, its initialization and dependencies, buffer conditions, completion notification, and timeout/error conventions. The binding can use either a DMA-backed RTOS bus API or an explicit BSP/HAL DMA interface.

**API Plan** defines when an operation is complete and when its buffers and results are valid. **Test Plan** retains the required device transactions and outputs, with completion ordering and error behavior treated as additional implementation requirements. Synthesis and repair then use these constraints to construct the driver. This division lets EDG reuse device knowledge while adapting the implementation to the target's transfer APIs.

### 6.3 Subset selection and hardware scope

The complete [30 DMA task packages](EDG_DMA_Tasks/README.md) and their [index](EDG_DMA_Tasks/index.json) accompany this supplement.

The subset contains **30 device–RTOS tasks, 22 devices, and nine RTOSes**, selected from the existing 325-task benchmark. Each task retains its original board/MCU target, device connection, bus type, and bus instance. Selection required an existing DMA path for that controller in the project's RTOS/BSP/HAL sources. Enabling existing DMA, interrupt, or device-tree configuration was allowed; replacing the board or implementing a missing low-level controller driver was outside this subset.

The selection covers all **22 devices using SPI, I²C, or UART** in the benchmark. W25Q64JV is repeated across eight RTOSes to examine platform binding for a common SPI device. MH-Z19B appears on two RTOSes to cover UART request/response handling. Each of the other 20 devices appears once, providing short transfers, register operations, multiple channels, numerical conversion, display data, and memory access.

### Table 9. Bus and device coverage of the DMA subset

| Bus | Distinct devices | Tasks | Share of 30 tasks |
| --- | ---: | ---: | ---: |
| SPI | 4 | 11 | 36.67% |
| I²C | 17 | 17 | 56.67% |
| UART | 1 | 2 | 6.67% |
| Total | 22 | 30 | 100.00% |

### Table 10. Selected tasks grouped by RTOS

| RTOS | Original target and retained interfaces | Devices, grouped by bus | Tasks |
| --- | --- | --- | ---: |
| FreeRTOS | STM32F103RB; SPI1 / I²C1 | SPI: W25Q64JV. I²C: SSD1306, BH1750, EMC1413, PCF8574. | 5 |
| CMSIS-RTX | STM32F103RB; SPI1 / I²C1 | SPI: ADXL345, W25Q64JV. I²C: AT24C256, LM75A, MCP23017. | 5 |
| ThreadX | STM32F103RB; `platform_spi_ops` / I²C1 | SPI: MAX31855, W25Q64JV. I²C: BME280, DPS310, TMP421. | 5 |
| TobudOS | ALIENTEK-MiniSTM32F103RCT6; SPI1 | SPI: MCP3008, W25Q64JV. | 2 |
| RT-Thread | STM32F103RB; `spi10` / `i2c1` | SPI: W25Q64JV. I²C: MPU6050, DS3231, PCA9685, SHT30. | 5 |
| NuttX | `nucleo-f103rb`; `/dev/ttyS1` | UART: MH-Z19B. | 1 |
| Zephyr | `stm32f103_mini`; SPI1 / original UART1 connection | SPI: W25Q64JV. UART: MH-Z19B. | 2 |
| ChibiOS | STM32F103RB-NUCLEO64; SPID1 / I2CD1 | SPI: W25Q64JV. I²C: LSM303DLHC, TMP105, VL53L0X. | 4 |
| Apache Mynewt | `generic_hal` target with STM32F103RB; SPI controller 0 | SPI: W25Q64JV. | 1 |
| Total | Original STM32F103-family targets | 22 distinct devices | 30 |

**Available DMA paths.** FreeRTOS, CMSIS-RTX, and ThreadX use their supplied/shared STM32CubeF1 support; TobudOS has its own F1 vendor HAL integration. RT-Thread provides F1 SPI DMA and a hardware-I²C DMA branch. NuttX provides USART RX/TX DMA on the selected F103 target. Zephyr provides STM32F1 DMA support for the selected SPI and UART paths. ChibiOS supplies STM32 SPIv1/I2Cv1 DMA implementations. Apache Mynewt provides an STM32 SPI bus driver with F1 DMA mappings for the retained controller. These paths cover both DMA-backed bus abstractions and explicit platform DMA interfaces.

**Exclusions.** Screening used the checked-out sources and original targets:

- RIOT's original `nucleo-f103rb` BSP lacks the required DMA configuration and SPI DMA mapping. RTEMS lacks an F103 BSP for the original target. XiUOS's original F103-nano BSP lacks an integrated SPI/DMA path. The supplied LiteOS-M target lacks the corresponding complete DMA BSP/SDK.
- NuttX SPI lacks the required hooks in the original board integration, and its selected STM32 I²C implementation lists DMA as unfinished. Zephyr's F103 I²C V1 path and Apache Mynewt's selected F103 I²C path lack the needed DMA integration. RT-Thread I²C tasks use the existing hardware-I²C DMA branch.
- DHT22, DS18B20, and HC-SR04 use GPIO timing interfaces in the benchmark. Their inclusion would require a separate timer/capture or waveform design, so they are outside this conventional bus-transfer experiment.

Short transfers remain eligible when supported: small payloads test binding and completion correctness, while memory and display devices provide longer-transfer use cases.

### 6.4 Aggregate results and polling comparison

Both modes are summarized over the **same 30 tasks**. Pass rates use the paper's cumulative rule: a task passes Lk only if it passes every level from L1 through Lk. The denominator remains 30 at every level, including generation or build failures. Rates equal the passing count divided by 30; differences below are DMA minus polling.

The polling column uses the current task records, including the completed ThreadX–W25Q64JV supplementary run, which reaches L3. The DMA column reports the supplementary experiment's aggregate counts.

### Table 11. Cumulative validation results: DMA versus polling

| Level | Polling passed / 30 | Polling rate | DMA passed / 30 | DMA rate | Count difference | Difference (pp) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| L1: build | 25 | 83.33% | 23 | 76.67% | −2 | −6.67 |
| L2: boot and execution | 22 | 73.33% | 22 | 73.33% | 0 | 0.00 |
| L3: protocol | 20 | 66.67% | 18 | 60.00% | −2 | −6.67 |
| L4: semantics | 15 | 50.00% | 16 | 53.33% | +1 | +3.33 |
| L5: robustness | 14 | 46.67% | 13 | 43.33% | −1 | −3.33 |

### 6.5 Interpretation

**Correctness remains close to polling across the full ladder.** DMA matches polling at L2, exceeds it by one task at L4, and reaches L5 on **13 tasks versus 14**, a difference of **3.33 percentage points**. The results provide additional evidence that EDG can generate drivers using supported DMA paths across varied device protocols and RTOS interfaces.

**The results identify several points for improvement.** Seven DMA tasks do not reach L1; among the 23 that pass L1, one falls short of L2, four more fall short of L3, two more fall short of L4, and three more fall short of L5. Thus, both initial integration and later behavioral correctness remain relevant. Relative to polling, the lower L1 count motivates closer examination of DMA configuration and binding, while the three-task drop from DMA L4 to L5 motivates stronger completion, timeout, and error-recovery checks.

**DMA fits EDG's separation of device knowledge and platform implementation.** The device protocol remains reusable, while the target contract captures how data is transferred and when results become valid. The observed L5 gap is one task, suggesting that the added transfer-management requirements are manageable within this design on the selected targets. Future benchmark extensions can broaden controller/BSP families and transfer modes, and add focused checks for interrupted transfers, buffer reuse, and repeated operations after errors. CPU occupancy, latency, and throughput measurements would complement the correctness comparison.
