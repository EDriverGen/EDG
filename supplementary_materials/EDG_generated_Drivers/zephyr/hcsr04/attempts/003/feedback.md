## ATTEMPT 3 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_hcsr04_dwqcvmvh/drv/hcsr04.c:9:83: error: macro "GPIO_DT_SPEC_GET" requires 3 arguments, but only 2 given
  - /tmp/stubcc_hcsr04_dwqcvmvh/drv/hcsr04.c:9:46: error: 'GPIO_DT_SPEC_GET' undeclared here (not in a function)
  - /tmp/stubcc_hcsr04_dwqcvmvh/drv/hcsr04.c:10:83: error: macro "GPIO_DT_SPEC_GET" requires 3 arguments, but only 2 given

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
