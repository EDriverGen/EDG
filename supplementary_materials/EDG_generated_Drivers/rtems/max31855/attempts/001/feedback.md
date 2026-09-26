## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_max31855_ab93t2os/drv/max31855.h:8:5: error: unknown type name 'spi_bus'
  - /tmp/stubcc_max31855_ab93t2os/drv/max31855.h:11:48: error: unknown type name 'spi_bus'
  - /tmp/stubcc_max31855_ab93t2os/drv/max31855.c:21:15: error: implicit declaration of function 'spi_bus_ioctl' [-Wimplicit-function-declaration]
  - /tmp/stubcc_max31855_ab93t2os/drv/max31855.c:28:48: error: unknown type name 'spi_bus'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
