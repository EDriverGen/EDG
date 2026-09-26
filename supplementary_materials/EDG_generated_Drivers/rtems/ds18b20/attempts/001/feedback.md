## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_ds18b20_52az4g2e/drv/ds18b20.c:26:38: error: 'RTEMS_GPIO_PIN_OUTPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_ds18b20_52az4g2e/drv/ds18b20.c:26:10: error: too many arguments to function 'rtems_gpio_request_pin'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
