## ATTEMPT 1 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:23:38: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:23:47: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:23:15: error: too many arguments to function 'I2C_TRANSFER'
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:39:28: error: 'dev' undeclared (first use in this function)
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:39:38: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:39:47: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:39:15: error: too many arguments to function 'I2C_TRANSFER'
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:52:38: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:52:47: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_sht30_t4s3edeg/drv/sht30.c:52:15: error: too many arguments to function 'I2C_TRANSFER'

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
