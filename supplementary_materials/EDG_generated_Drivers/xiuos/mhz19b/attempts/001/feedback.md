## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_mhz19b_pjw1bez6/drv/mhz19b.c:18:12: error: implicit declaration of function 'write' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_pjw1bez6/drv/mhz19b.c:23:12: error: implicit declaration of function 'read' [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
