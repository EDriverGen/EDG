## ATTEMPT 2 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - driver_source/header line 45: function-like symbol `i2c_write` looks like an RTOS/vendor-SDK or bus API, but SECTION C does not list it for this task. Treat the RTOS contract as a call allow-list and use only the bound symbol/signature(s): `OK`, `int i2cdev_open(int bus)`, `int i2cdev_transfer(int fd, FAR struct i2c_msg_s *msgv, int msgc)`, `ssize_t i2ctool_write(FAR struct i2ctool_s *i2ctool, FAR const void *buffer, size_t nbytes)`, `int_status_return`, `ssize_t`, `void weak_function up_mdelay(unsigned int milliseconds)`. If this is meant to be a private helper, define it with a device-specific helper name outside the RTOS/bus namespace.
  - driver_source/header line 47: function-like symbol `i2c_read` looks like an RTOS/vendor-SDK or bus API, but SECTION C does not list it for this task. Treat the RTOS contract as a call allow-list and use only the bound symbol/signature(s): `OK`, `int i2cdev_open(int bus)`, `int i2cdev_transfer(int fd, FAR struct i2c_msg_s *msgv, int msgc)`, `ssize_t i2ctool_write(FAR struct i2ctool_s *i2ctool, FAR const void *buffer, size_t nbytes)`, `int_status_return`, `ssize_t`, `void weak_function up_mdelay(unsigned int milliseconds)`. If this is meant to be a private helper, define it with a device-specific helper name outside the RTOS/bus namespace.
  - driver_source/header line 57: function-like symbol `i2c_write` looks like an RTOS/vendor-SDK or bus API, but SECTION C does not list it for this task. Treat the RTOS contract as a call allow-list and use only the bound symbol/signature(s): `OK`, `int i2cdev_open(int bus)`, `int i2cdev_transfer(int fd, FAR struct i2c_msg_s *msgv, int msgc)`, `ssize_t i2ctool_write(FAR struct i2ctool_s *i2ctool, FAR const void *buffer, size_t nbytes)`, `int_status_return`, `ssize_t`, `void weak_function up_mdelay(unsigned int milliseconds)`. If this is meant to be a private helper, define it with a device-specific helper name outside the RTOS/bus namespace.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
