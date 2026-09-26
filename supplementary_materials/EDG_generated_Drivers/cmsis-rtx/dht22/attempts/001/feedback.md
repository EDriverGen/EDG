## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dht22_7f2qxhur/drv/dht22.c:19:5: error: implicit declaration of function '__HAL_RCC_GPIOB_CLK_ENABLE' [-Wimplicit-function-declaration]
  - /tmp/stubcc_dht22_7f2qxhur/drv/dht22.c:21:28: error: 'GPIO_MODE_OUTPUT_OD' undeclared (first use in this function); did you mean 'GPIO_MODE_OUTPUT_PP'?
  - /tmp/stubcc_dht22_7f2qxhur/drv/dht22.c:23:29: error: 'GPIO_SPEED_FREQ_HIGH' undeclared (first use in this function); did you mean 'GPIO_SPEED_FREQ_LOW'?

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
