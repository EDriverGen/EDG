## ATTEMPT 1 FEEDBACK

### Compile errors
- *SYMBOL SURFACE MISMATCH*: the compiler found a likely public replacement for an undeclared symbol. Treat the undeclared symbol as forbidden in the next attempt when the replacement appears in SECTION C or a public/stub header:
    - replace forbidden `sleep` with public `usleep`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:20:17: error: too few arguments to function 'rtems_gpio_get_value'
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:31:38: error: 'RTEMS_GPIO_PIN_OUTPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:31:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:33:5: error: implicit declaration of function 'sleep'; did you mean 'usleep'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:42:33: error: 'RTEMS_GPIO_PIN_OUTPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_OUTPUT'?
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:42:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:48:33: error: 'RTEMS_GPIO_PIN_INPUT' undeclared (first use in this function); did you mean 'RTEMS_GPIO_INPUT'?
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:48:5: error: too many arguments to function 'rtems_gpio_request_pin'
  - /tmp/stubcc_dht22_kucb1g89/drv/dht22.c:61:16: error: too few arguments to function 'rtems_gpio_get_value'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
