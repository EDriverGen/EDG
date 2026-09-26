## ATTEMPT 3 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_mcp3008_9d74netn/drv/mcp3008.h:8:5: error: unknown type name 'spi_bus'
  - /tmp/stubcc_mcp3008_9d74netn/drv/mcp3008.h:11:47: error: unknown type name 'spi_bus'
  - /tmp/stubcc_mcp3008_9d74netn/drv/mcp3008.c:5:47: error: unknown type name 'spi_bus'
  - /tmp/stubcc_mcp3008_9d74netn/drv/mcp3008.c:17:5: error: implicit declaration of function 'spi_bus_transfer' [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
