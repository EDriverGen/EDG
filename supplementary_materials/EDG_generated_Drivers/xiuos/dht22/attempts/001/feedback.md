## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dht22_qsa_wneq/drv/dht22.c:30:9: error: implicit declaration of function 'usleep' [-Wimplicit-function-declaration]
  - /tmp/stubcc_dht22_qsa_wneq/drv/dht22.c:38:20: error: implicit declaration of function 'open' [-Wimplicit-function-declaration]
  - /tmp/stubcc_dht22_qsa_wneq/drv/dht22.c:41:32: error: implicit declaration of function 'close' [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
