## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dps310_sodzh9pa/drv/dps310.c:23:53: error: 'OS_TICK_PER_SECOND' undeclared (first use in this function); did you mean 'OS_TICKS_PER_SEC'?
  - /tmp/stubcc_dps310_sodzh9pa/drv/dps310.c:38:55: error: 'OS_TICK_PER_SECOND' undeclared (first use in this function); did you mean 'OS_TICKS_PER_SEC'?
  - /tmp/stubcc_dps310_sodzh9pa/drv/dps310.c:49:19: error: 'OS_TICK_PER_SECOND' undeclared (first use in this function); did you mean 'OS_TICKS_PER_SEC'?

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
