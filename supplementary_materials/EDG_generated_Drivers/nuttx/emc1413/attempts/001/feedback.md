## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:29:38: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:29:47: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:29:15: error: too many arguments to function 'I2C_TRANSFER'
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:42:38: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:42:47: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:42:15: error: too many arguments to function 'I2C_TRANSFER'
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:46:34: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:46:43: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_emc1413_i0zhgdjx/drv/emc1413.c:46:11: error: too many arguments to function 'I2C_TRANSFER'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
