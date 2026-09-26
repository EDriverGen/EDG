## ATTEMPT 2 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dht22_9rxh16c4/drv/dht22.c:14:19: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_dht22_9rxh16c4/drv/dht22.c:27:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_9rxh16c4/drv/dht22.c:39:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_9rxh16c4/drv/dht22.c:45:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_9rxh16c4/drv/dht22.c:66:16: error: too few arguments to function 'rtems_gpio_get_value'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
