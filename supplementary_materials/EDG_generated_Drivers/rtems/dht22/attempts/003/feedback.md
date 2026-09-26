## ATTEMPT 3 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:22:43: error: 'RTEMS_GPIO_PIN_OUTPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:22:10: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:34:12: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:39:15: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:40:12: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:52:33: error: 'RTEMS_GPIO_PIN_OUTPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:52:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:58:33: error: 'RTEMS_GPIO_PIN_INPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_INPUT'?
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:58:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:60:12: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:65:12: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_dht22_jx2vfbib/drv/dht22.c:70:12: error: too few arguments to function 'rtems_gpio_get_value'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
