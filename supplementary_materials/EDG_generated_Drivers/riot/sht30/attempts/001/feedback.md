## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - driver_source uses scaled conversion coefficient(s) 100000, 175000 from Device IR milli/micro-unit formulae without any `int64_t`, `uint64_t`, or `long long` intermediate. Promote the raw-code multiplication/division to a 64-bit intermediate before assigning back to the public output type; 16-bit or wider raw fields can overflow `int32_t` during `raw * scale` even when the final result fits.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
