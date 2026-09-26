#include "lm75a.h"
#include "nuttx.h"
#include <stdint.h>

#define LM75A_ADDR 0x48
#define LM75A_TEMP_REG 0x00

int lm75a_init(struct lm75a_dev *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = LM75A_ADDR;
    up_mdelay(100);
    return 0;
}

int lm75a_read_temp(struct lm75a_dev *dev, int32_t *raw)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;

    uint8_t cmd = LM75A_TEMP_REG;
    int ret = i2c_write(dev->bus, &config, &cmd, 1);
    if (ret < 0) {
        return -1;
    }

    uint8_t buf[2];
    ret = i2c_read(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return -1;
    }

    int16_t reg = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw11 = (reg >> 5) & 0x7FF;
    if (raw11 & 0x400) {
        raw11 |= 0xF800;
    }
    *raw = (int32_t)raw11;
    return 0;
}
