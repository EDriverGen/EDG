## ATTEMPT 2 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_hcsr04_0m94bv6c/drv/hcsr04.c:18:43: error: 'RTEMS_GPIO_FUNCTION' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_hcsr04_0m94bv6c/drv/hcsr04.c:18:10: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_hcsr04_0m94bv6c/drv/hcsr04.c:22:10: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_hcsr04_0m94bv6c/drv/hcsr04.c:36:15: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_hcsr04_0m94bv6c/drv/hcsr04.c:46:15: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_hcsr04_0m94bv6c/drv/hcsr04.c:69:43: error: 'RTEMS_GPIO_FUNCTION' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_hcsr04_0m94bv6c/drv/hcsr04.c:69:5: error: too many arguments to function 'rtems_gpio_request_pin'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
