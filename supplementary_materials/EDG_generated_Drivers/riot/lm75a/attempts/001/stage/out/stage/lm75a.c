#include "lm75a.h"
#include "periph/i2c.h"
#include "ztimer.h"
#include <stddef.h>

#define LM75A_REG_TEMP 0x00

int lm75a_init(lm75a_t *dev, i2c_t bus, uint16_t addr)
{
    dev->bus = bus;
    dev->addr = addr;
    ztimer_sleep(ZTIMER_MSEC, 100);
    return 0;
}

int lm75a_read_temperature(lm75a_t *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;

    i2c_acquire(dev->bus);
    ret = i2c_read_regs(dev->bus, dev->addr, LM75A_REG_TEMP, buf, 2, 0);
    i2c_release(dev->bus);
    if (ret < 0) {
        return ret;
    }

    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t raw11 = (int16_t)(raw16 >> 5);
    if (raw11 & 0x0400) {
        raw11 |= 0xF800;
    }
    *raw = (int32_t)raw11 * 125;
    return 0;
}