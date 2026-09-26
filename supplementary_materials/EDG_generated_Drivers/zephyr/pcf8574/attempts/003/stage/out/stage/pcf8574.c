#include "pcf8574.h"
#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/sys/byteorder.h>
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(const struct device *dev, const struct device *bus)
{
    (void)dev;
    (void)bus;
    return 0;
}

int pcf8574_read_port(const struct device *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7)
{
    const struct i2c_dt_spec spec = { .bus = dev, .addr = PCF8574_I2C_ADDR };
    uint8_t buf;
    int ret;

    ret = i2c_read_dt(&spec, &buf, 1);
    if (ret != 0) {
        return ret;
    }

    *p0 = (buf >> 0) & 1;
    *p1 = (buf >> 1) & 1;
    *p2 = (buf >> 2) & 1;
    *p3 = (buf >> 3) & 1;
    *p4 = (buf >> 4) & 1;
    *p5 = (buf >> 5) & 1;
    *p6 = (buf >> 6) & 1;
    *p7 = (buf >> 7) & 1;

    return 0;
}
