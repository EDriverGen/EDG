## ATTEMPT 1 FEEDBACK

### Compile errors
- *SYMBOL SURFACE MISMATCH*: the compiler found a likely public replacement for an undeclared symbol. Treat the undeclared symbol as forbidden in the next attempt when the replacement appears in SECTION C or a public/stub header:
    - replace forbidden `I2C_WRITE` with public `GPIOC_WRITE`
    - replace forbidden `I2C_READ` with public `I2C_M_READ`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_mpu6050_x1zmrlva/drv/mpu6050.c:18:38: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_mpu6050_x1zmrlva/drv/mpu6050.c:18:47: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_mpu6050_x1zmrlva/drv/mpu6050.c:18:15: error: too many arguments to function 'I2C_TRANSFER'
  - /tmp/stubcc_mpu6050_x1zmrlva/drv/mpu6050.c:31:15: error: implicit declaration of function 'I2C_WRITE'; did you mean 'GPIOC_WRITE'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_mpu6050_x1zmrlva/drv/mpu6050.c:35:11: error: implicit declaration of function 'I2C_READ'; did you mean 'I2C_M_READ'? [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
