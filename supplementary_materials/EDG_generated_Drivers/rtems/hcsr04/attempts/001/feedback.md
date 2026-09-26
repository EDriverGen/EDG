## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_hcsr04_0oisgpep/drv/hcsr04.c:18:43: error: 'RTEMS_GPIO_PIN_OUTPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_hcsr04_0oisgpep/drv/hcsr04.c:18:10: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_hcsr04_0oisgpep/drv/hcsr04.c:21:43: error: 'RTEMS_GPIO_PIN_INPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_INPUT'?
  - /tmp/stubcc_hcsr04_0oisgpep/drv/hcsr04.c:21:10: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_hcsr04_0oisgpep/drv/hcsr04.c:35:15: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_hcsr04_0oisgpep/drv/hcsr04.c:45:15: error: too few arguments to function 'rtems_gpio_get_value'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
