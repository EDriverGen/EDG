## ATTEMPT 1 FEEDBACK

### Compile errors
- *SYMBOL SURFACE MISMATCH*: the compiler found a likely public replacement for an undeclared symbol. Treat the undeclared symbol as forbidden in the next attempt when the replacement appears in SECTION C or a public/stub header:
    - replace forbidden `I2C_WRITE` with public `GPIOC_WRITE`
    - replace forbidden `I2C_READ` with public `I2C_M_READ`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_vl53l0x_o7bry0i0/drv/vl53l0x.c:18:15: error: implicit declaration of function 'I2C_WRITE'; did you mean 'GPIOC_WRITE'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_vl53l0x_o7bry0i0/drv/vl53l0x.c:24:11: error: implicit declaration of function 'I2C_READ'; did you mean 'I2C_M_READ'? [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
