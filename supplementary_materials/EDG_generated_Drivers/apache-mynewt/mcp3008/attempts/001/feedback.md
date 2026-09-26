## ATTEMPT 1 FEEDBACK

### Compile errors
- *STRUCT FIELD MISMATCH*: the previous attempt used field names that are not present in the RTOS struct definition. In the next attempt, use only the struct fields rendered in SECTION C's RTOS struct field allow-list:
    - `struct hal_spi_settings` has no field `data_mode`
    - `struct hal_spi_settings` has no field `data_order`
    - `struct hal_spi_settings` has no field `word_size`
    - `struct hal_spi_settings` has no field `baudrate`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:25:12: error: variable 'spi_settings' has initializer but incomplete type
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:26:10: error: 'struct hal_spi_settings' has no member named 'data_mode'
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:26:22: error: 'HAL_SPI_MODE0' undeclared (first use in this function)
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:27:10: error: 'struct hal_spi_settings' has no member named 'data_order'
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:27:23: error: 'HAL_SPI_MSB_FIRST' undeclared (first use in this function)
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:28:10: error: 'struct hal_spi_settings' has no member named 'word_size'
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:28:22: error: 'HAL_SPI_WORD_SIZE_8BIT' undeclared (first use in this function)
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:29:10: error: 'struct hal_spi_settings' has no member named 'baudrate'
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:25:29: error: storage size of 'spi_settings' isn't known
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:32:10: error: implicit declaration of function 'hal_spi_config' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mcp3008_4noa0s6s/drv/mcp3008.c:35:10: error: implicit declaration of function 'hal_spi_txrx' [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
