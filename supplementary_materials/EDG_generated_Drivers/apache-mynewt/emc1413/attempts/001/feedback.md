## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_emc1413_bxnhwwic/drv/emc1413.c:28:52: error: 'OS_TICK_PER_SECOND' undeclared (first use in this function); did you mean 'OS_TICKS_PER_SEC'?
  - /tmp/stubcc_emc1413_bxnhwwic/drv/emc1413.c:46:54: error: 'OS_TICK_PER_SECOND' undeclared (first use in this function); did you mean 'OS_TICKS_PER_SEC'?
  - /tmp/stubcc_emc1413_bxnhwwic/drv/emc1413.c:78:19: error: 'OS_TICK_PER_SECOND' undeclared (first use in this function); did you mean 'OS_TICKS_PER_SEC'?

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
