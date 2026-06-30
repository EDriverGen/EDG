#ifndef DRIVERGEN_RTEMS_LINUX_I2C_H
#define DRIVERGEN_RTEMS_LINUX_I2C_H

#include <stdint.h>

#define I2C_M_TEN 0x0010
#define I2C_M_RD  0x0001
#define I2C_M_STOP 0x8000
#define I2C_M_NOSTART 0x4000

struct i2c_msg {
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t *buf;
};

#define I2C_FUNC_I2C 0x00000001
#define I2C_SMBUS_BLOCK_MAX 32

union i2c_smbus_data {
    uint8_t byte;
    uint16_t word;
    uint8_t block[I2C_SMBUS_BLOCK_MAX + 2];
};

#endif
