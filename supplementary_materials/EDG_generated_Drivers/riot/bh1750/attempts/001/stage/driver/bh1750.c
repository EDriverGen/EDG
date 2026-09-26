#include "bh1750.h"
#include <stdint.h>
#include <string.h>
#include "periph/i2c.h"
#include "ztimer.h"

#define BH1750_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_H_RES 0x10
#define BH1750_DELAY_MS 180

int bh1750_init(bh1750_device_t *dev, i2c_t bus)
{
    dev->bus = bus;
    dev->addr = BH1750_ADDR;
    dev->mtreg = 69;

    i2c_acquire(dev->bus);
    int ret = i2c_write_byte(dev->bus, dev->addr, BH1750_CMD_POWER_ON, 0);
    i2c_release(dev->bus);
    return ret;
}

int bh1750_read_illuminance(bh1750_device_t *dev, uint16_t *raw)
{
    uint8_t buf[2];
    int ret;

    i2c_acquire(dev->bus);
    ret = i2c_write_byte(dev->bus, dev->addr, BH1750_CMD_CONT_H_RES, 0);
    if (ret != 0) {
        i2c_release(dev->bus);
        return ret;
    }
    ztimer_sleep(ZTIMER_MSEC, BH1750_DELAY_MS);
    ret = i2c_read_bytes(dev->bus, dev->addr, buf, 2, 0);
    i2c_release(dev->bus);
    if (ret != 0) {
        return ret;
    }
    *raw = ((uint16_t)buf[0] << 8) | buf[1];
    return 0;
}
