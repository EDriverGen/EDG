#include "lm75a.h"
#include "zephyr.h"
#include <stdint.h>

#define LM75A_I2C_ADDR 0x48
#define LM75A_REG_TEMP 0x00

int lm75a_init(const struct device *dev)
{
    if (!device_is_ready(dev)) {
        return -1;
    }
    k_msleep(100);
    return 0;
}

int lm75a_read_temp(const struct device *dev, int32_t *raw)
{
    uint8_t cmd = LM75A_REG_TEMP;
    uint8_t buf[2];
    int ret;

    ret = i2c_write(dev, &cmd, 1, LM75A_I2C_ADDR);
    if (ret != 0) {
        return -1;
    }

    ret = i2c_read(dev, buf, 2, LM75A_I2C_ADDR);
    if (ret != 0) {
        return -1;
    }

    int32_t raw_val = ((int32_t)buf[0] << 8) | buf[1];
    raw_val >>= 5;
    if (raw_val & 0x400) {
        raw_val |= ~0x7FF;
    }
    *raw = raw_val;
    return 0;
}