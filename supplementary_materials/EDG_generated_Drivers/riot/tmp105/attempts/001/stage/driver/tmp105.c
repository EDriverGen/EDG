#include "tmp105.h"
#include "xtimer.h"
#include <errno.h>

#include "riot.h"
#define TMP105_POINTER_REG 0x00

int tmp105_init(tmp105_t *dev, i2c_t bus, uint8_t addr)
{
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int tmp105_read_temperature(tmp105_t *dev, int32_t *raw)
{
    uint8_t reg = TMP105_POINTER_REG;
    uint8_t buf[2];
    int ret;

    xtimer_msleep(220);

    ret = i2c_read_regs(dev->bus, dev->addr, reg, buf, 2, 0);
    if (ret < 0) {
        return -EIO;
    }

    int16_t raw12 = (int16_t)(((buf[0] << 8) | buf[1]) >> 4);
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }
    *raw = ((int32_t)raw12 * 125) / 2;
    return 0;
}
