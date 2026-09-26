## ATTEMPT 1 FEEDBACK

### Compile errors
- *SYMBOL SURFACE MISMATCH*: the compiler found a likely public replacement for an undeclared symbol. Treat the undeclared symbol as forbidden in the next attempt when the replacement appears in SECTION C or a public/stub header:
    - replace forbidden `uartSendTimeout` with public `sdReadTimeout`
    - replace forbidden `uartReceiveTimeout` with public `i2cMasterReceiveTimeout`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_mhz19b_qo7r0laq/drv/mhz19b.c:34:14: error: implicit declaration of function 'uartSendTimeout'; did you mean 'sdReadTimeout'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_qo7r0laq/drv/mhz19b.c:34:31: error: 'UARTDriver' undeclared (first use in this function)
  - /tmp/stubcc_mhz19b_qo7r0laq/drv/mhz19b.c:34:43: error: expected expression before ')' token
  - /tmp/stubcc_mhz19b_qo7r0laq/drv/mhz19b.c:40:14: error: implicit declaration of function 'uartReceiveTimeout'; did you mean 'i2cMasterReceiveTimeout'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_qo7r0laq/drv/mhz19b.c:40:46: error: expected expression before ')' token

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
