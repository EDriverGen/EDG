## ATTEMPT 1 FEEDBACK

### Compile errors
- *SYMBOL SURFACE MISMATCH*: the compiler found a likely public replacement for an undeclared symbol. Treat the undeclared symbol as forbidden in the next attempt when the replacement appears in SECTION C or a public/stub header:
    - replace forbidden `GPIO_DT_SPEC_GET_BY_IDX` with public `GPIO_DT_SPEC_GET`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_max31855_r2bxlo50/drv/max31855.c:18:17: error: field name not in record or union initializer
  - /tmp/stubcc_max31855_r2bxlo50/drv/max31855.c:18:25: error: implicit declaration of function 'GPIO_DT_SPEC_GET_BY_IDX'; did you mean 'GPIO_DT_SPEC_GET'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_max31855_r2bxlo50/drv/max31855.c:18:69: error: 'cs_gpios' undeclared (first use in this function)
  - /tmp/stubcc_max31855_r2bxlo50/drv/max31855.c:19:17: error: field name not in record or union initializer

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
