## ATTEMPT 1 FEEDBACK

### Compile errors
- *SYMBOL SURFACE MISMATCH*: the compiler found a likely public replacement for an undeclared symbol. Treat the undeclared symbol as forbidden in the next attempt when the replacement appears in SECTION C or a public/stub header:
    - replace forbidden `I2C_WRITEREAD` with public `I2C_M_READ`
    - replace forbidden `I2C_WRITE` with public `GPIOC_WRITE`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_dps310_d6rxr02l/drv/dps310.c:25:15: error: implicit declaration of function 'I2C_WRITEREAD'; did you mean 'I2C_M_READ'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_dps310_d6rxr02l/drv/dps310.c:39:15: error: implicit declaration of function 'I2C_WRITE'; did you mean 'GPIOC_WRITE'? [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
