## ATTEMPT 3 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_ds18b20_trm9007d/drv/ds18b20.c:40:15: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_ds18b20_trm9007d/drv/ds18b20.c:65:15: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_ds18b20_trm9007d/drv/ds18b20.c:89:38: error: 'RTEMS_GPIO_FUNCTION_NONE' undeclared (first use in this function)
  - /tmp/stubcc_ds18b20_trm9007d/drv/ds18b20.c:89:5: error: too many arguments to function 'rtems_gpio_request_pin'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
