#include "lm75a.h"
#include <stdint.h>
#include <string.h>

#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG 0x00

int32_t lm75a_init(struct lm75a_device *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    OsalMSleep(100);
    return 0;
}

int32_t lm75a_read_temp(struct lm75a_device *dev, int32_t *raw)
{
    if (dev == NULL || raw == NULL) {
        return -1;
    }
    uint8_t cmd = LM75A_TEMP_REG;
    struct I2cMsg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &cmd;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    uint8_t buf[2];
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = 2;
    msgs[1].flags = 0x01;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return -1;
    }
    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t raw11 = (int16_t)(raw16 >> 5);
    if (raw11 & 0x0400) {
        raw11 |= 0xF800;
    }
    *raw = (int32_t)raw11;
    return 0;
}
