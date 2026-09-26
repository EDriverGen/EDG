#include "ds3231.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
#define DS3231_I2C_ADDR 0x68

static int ds3231_write_reg(const struct device *dev, uint8_t reg, const uint8_t *data, uint32_t len)
{
    uint8_t buf[len + 1];
    buf[0] = reg;
    for (uint32_t i = 0; i < len; i++) {
        buf[i + 1] = data[i];
    }
    struct i2c_msg msg = {
        .buf = buf,
        .len = len + 1,
        .flags = I2C_MSG_WRITE,
    };
    return i2c_transfer(dev, &msg, 1, DS3231_I2C_ADDR);
}

static int ds3231_read_reg(const struct device *dev, uint8_t reg, uint8_t *data, uint32_t len)
{
    struct i2c_msg msgs[2] = {
        {
            .buf = &reg,
            .len = 1,
            .flags = I2C_MSG_WRITE,
        },
        {
            .buf = data,
            .len = len,
            .flags = I2C_MSG_READ | I2C_MSG_STOP,
        },
    };
    return i2c_transfer(dev, msgs, 2, DS3231_I2C_ADDR);
}

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(const struct device *dev)
{
    if (!device_is_ready(dev)) {
        return -ENODEV;
    }
    return 0;
}

int ds3231_get_time(const struct device *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    int ret = ds3231_read_reg(dev, 0x00, buf, 7);
    if (ret < 0) {
        return ret;
    }
    t->second = bcd_to_dec(buf[0] & 0x7F);
    t->minute = bcd_to_dec(buf[1] & 0x7F);
    t->hour = bcd_to_dec(buf[2] & 0x3F);
    t->weekday = buf[3] & 0x07;
    t->day = bcd_to_dec(buf[4] & 0x3F);
    t->month = bcd_to_dec(buf[5] & 0x1F);
    t->year = bcd_to_dec(buf[6]);
    return 0;
}

int ds3231_set_time(const struct device *dev, const struct ds3231_time *t)
{
    uint8_t buf[7];
    buf[0] = dec_to_bcd(t->second) & 0x7F;
    buf[1] = dec_to_bcd(t->minute) & 0x7F;
    buf[2] = dec_to_bcd(t->hour) & 0x3F;
    buf[3] = t->weekday & 0x07;
    buf[4] = dec_to_bcd(t->day) & 0x3F;
    buf[5] = dec_to_bcd(t->month) & 0x1F;
    buf[6] = dec_to_bcd(t->year);
    return ds3231_write_reg(dev, 0x00, buf, 7);
}
