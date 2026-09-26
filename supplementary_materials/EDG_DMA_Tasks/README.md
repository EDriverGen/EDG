# EDG DMA task packages

This collection contains **30 DMA task definitions covering 22 devices and nine RTOSes**: 11 SPI, 17 I2C, and two UART tasks. Each task preserves its original board, MCU, bus instance, device addressing, and connection.

See the [supplementary material, Section 6](../supplementary_material.md#6-dma-driver-generation-on-30-tasks) for task selection and the aggregate comparison with polling.

## Layout

    <rtos>/<device>/
        task_package.json
        platform_base_context.json
        connection_binding_context.json
        device_attachment_context.json
        board_context.json
        fixed_task_context.json
        target_binding.json
        device_ir.json
        dma_requirements.json
        evaluation_config.json
        oracle/
    index.json
    README.md

The package uses the framework's existing platform + connection + device context schema. Device IR and device-oracle inputs accompany each task. DMA requirements specify the selected backend, configuration, completion and buffer rules, device-specific operations, and validation checks.

The task and connection IDs end in `_dma`; `mode` is `dma`. UART connections use `uart_dma`. MCU completion IRQs are enabled independently of any external device IRQ. The task requests regeneration of RTOS Contract, API/Test Plans, and driver code for the DMA configuration.

## Use with DriverGen

Place this `EDG_DMA_Tasks` directory at the DriverGen project root, alongside `drivergen/` and `data/`. Context references use project-relative paths. Keep the existing datasheets, RTOS/BSP sources, and evaluation tools available through the normal project setup.

Run a task using its file path:

```bash
python -m drivergen run --task-package EDG_DMA_Tasks/freertos/w25q64jv/task_package.json --provider deepseek --model deepseek-v4-flash --codegen --max-repairs 2
```

`index.json` lists the 30 task paths. `evaluation_config.json` identifies the task's local device-oracle files and the additional DMA checks.

## Backend-specific settings

- FreeRTOS, CMSIS-RTX, ThreadX, and TobudOS select the original STM32F1 HAL DMA backend and keep their board handles or operation wrappers.
- RT-Thread SPI uses the existing DMA branch for payloads of at least 10 bytes. Its selected hardware-I2C tasks use payload messages of at least two bytes; short address/control phases follow the bus driver's normal sequencing.
- NuttX retains `/dev/ttyS1`; RX/TX DMA must match the USART registered at that device node in the original board configuration.
- Zephyr preserves the `stm32f103_mini` board. The original UART task label `uart1` resolves to the BSP's `usart1` node on PA9/PA10, with the original 9600-8N1 sensor settings.
- ChibiOS retains SPID1/I2CD1 on the original F103RB board and selects the existing STM32 HAL DMA implementation.
- Apache Mynewt retains controller 0 and selects its STM32 SPI bus driver. The existing RX/TX DMA thresholds default to four bytes.

Core payload transfers must use DMA. Completion/error handling and buffer lifetime follow the selected backend's contract; device protocol and timing requirements remain in force.

## Task list

| ID | RTOS | Device | Bus | Board | Task package |
| --- | --- | --- | --- | --- | --- |
| D01 | freertos | W25Q64JV | SPI | stm32f103rb | [task](freertos/w25q64jv/task_package.json) |
| D02 | freertos | SSD1306 | I2C | stm32f103rb | [task](freertos/ssd1306/task_package.json) |
| D03 | freertos | BH1750 | I2C | stm32f103rb | [task](freertos/bh1750/task_package.json) |
| D04 | freertos | EMC1413 | I2C | stm32f103rb | [task](freertos/emc1413/task_package.json) |
| D05 | freertos | PCF8574 | I2C | stm32f103rb | [task](freertos/pcf8574/task_package.json) |
| D06 | cmsis-rtx | ADXL345 | SPI | stm32f103rb | [task](cmsis-rtx/adxl345/task_package.json) |
| D07 | cmsis-rtx | AT24C256 | I2C | stm32f103rb | [task](cmsis-rtx/at24c256/task_package.json) |
| D08 | cmsis-rtx | LM75A | I2C | stm32f103rb | [task](cmsis-rtx/lm75a/task_package.json) |
| D09 | cmsis-rtx | MCP23017 | I2C | stm32f103rb | [task](cmsis-rtx/mcp23017/task_package.json) |
| D10 | threadx | MAX31855 | SPI | stm32f103rb | [task](threadx/max31855/task_package.json) |
| D11 | threadx | BME280 | I2C | stm32f103rb | [task](threadx/bme280/task_package.json) |
| D12 | threadx | DPS310 | I2C | stm32f103rb | [task](threadx/dps310/task_package.json) |
| D13 | threadx | TMP421 | I2C | stm32f103rb | [task](threadx/tmp421/task_package.json) |
| D14 | tobudos | MCP3008 | SPI | ALIENTEK-MiniSTM32F103RCT6 | [task](tobudos/mcp3008/task_package.json) |
| D15 | rtthread | W25Q64JV | SPI | stm32f103rb | [task](rtthread/w25q64jv/task_package.json) |
| D16 | rtthread | MPU6050 | I2C | stm32f103rb | [task](rtthread/mpu6050/task_package.json) |
| D17 | rtthread | DS3231 | I2C | stm32f103rb | [task](rtthread/ds3231/task_package.json) |
| D18 | rtthread | PCA9685 | I2C | stm32f103rb | [task](rtthread/pca9685/task_package.json) |
| D19 | rtthread | SHT30 | I2C | stm32f103rb | [task](rtthread/sht30/task_package.json) |
| D20 | nuttx | MHZ19B | UART | nucleo-f103rb | [task](nuttx/mhz19b/task_package.json) |
| D21 | zephyr | W25Q64JV | SPI | stm32f103_mini | [task](zephyr/w25q64jv/task_package.json) |
| D22 | zephyr | MHZ19B | UART | stm32f103_mini | [task](zephyr/mhz19b/task_package.json) |
| D23 | cmsis-rtx | W25Q64JV | SPI | stm32f103rb | [task](cmsis-rtx/w25q64jv/task_package.json) |
| D24 | chibios | W25Q64JV | SPI | STM32F103RB-NUCLEO64 | [task](chibios/w25q64jv/task_package.json) |
| D25 | chibios | LSM303DLHC | I2C | STM32F103RB-NUCLEO64 | [task](chibios/lsm303dlhc/task_package.json) |
| D26 | chibios | TMP105 | I2C | STM32F103RB-NUCLEO64 | [task](chibios/tmp105/task_package.json) |
| D27 | chibios | VL53L0X | I2C | STM32F103RB-NUCLEO64 | [task](chibios/vl53l0x/task_package.json) |
| D28 | apache-mynewt | W25Q64JV | SPI | generic_hal | [task](apache-mynewt/w25q64jv/task_package.json) |
| D29 | threadx | W25Q64JV | SPI | stm32f103rb | [task](threadx/w25q64jv/task_package.json) |
| D30 | tobudos | W25Q64JV | SPI | ALIENTEK-MiniSTM32F103RCT6 | [task](tobudos/w25q64jv/task_package.json) |

## Evaluation

The five validation levels use the same cumulative rule as EDGBench: build, boot/execution, protocol, semantics, and robustness. DMA checks additionally cover actual payload submission/completion, buffer lifetime, bounded error handling, and subsequent operations. The supplement reports the aggregate experimental results.
