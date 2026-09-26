## ATTEMPT 2 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dht22_nt3gtea1/drv/dht22.c:24:9: error: implicit declaration of function 'usleep' [-Wimplicit-function-declaration]
  - /tmp/stubcc_dht22_nt3gtea1/drv/dht22.c:32:20: error: implicit declaration of function 'open' [-Wimplicit-function-declaration]
  - /tmp/stubcc_dht22_nt3gtea1/drv/dht22.c:36:9: error: implicit declaration of function 'close' [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
