#include "lm75a.h"
#include <stdint.h>

#include "rtthread.h"
#define LM75A_ADDR 0x48
#define LM75A_REG_TEMP 0x00

int lm75a_init(struct lm75a_device *dev, struct rt_i2c_bus_device *bus)
{
    if (dev == RT_NULL || bus == RT_NULL) {
        return -1;
    }
    dev->bus = bus;
    dev->addr = LM75A_ADDR;
    rt_thread_mdelay(100);
    return 0;
}

int lm75a_read_temperature(struct lm75a_device *dev, int32_t *raw)
{
    struct rt_i2c_msg msgs[2];
    uint8_t cmd = LM75A_REG_TEMP;
    uint8_t buf[2];
    int ret;

    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &cmd;

    msgs[1].addr = dev->addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = 2;
    msgs[1].buf = buf;

    ret = rt_i2c_transfer(dev->bus, msgs, 2);
    if (ret != 2) {
        return -1;
    }

    int16_t raw16 = ((int16_t)buf[0] << 8) | buf[1];
    *raw = (raw16 >> 5) & 0x7FF;
    return 0;
}
