#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "ds3231.h"

#include <zephyr/sys/byteorder.h>
#define DS3231_I2C_ADDR 0x68

static int ds3231_write_reg(const struct device *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    struct i2c_msg msgs[2];
    uint8_t buf[1 + len];
    buf[0] = reg;
    for (size_t i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    msgs[0].buf = buf;
    msgs[0].len = 1 + len;
    msgs[0].flags = I2C_MSG_WRITE;
    return i2c_transfer(dev, msgs, 1, DS3231_I2C_ADDR);
}

static int ds3231_read_reg(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
    struct i2c_msg msgs[2];
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = I2C_MSG_WRITE;
    msgs[1].buf = data;
    msgs[1].len = len;
    msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;
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
    if (!dev) {
        return -EINVAL;
    }
    return 0;
}

int ds3231_get_time(const struct device *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    int ret;

    if (!dev || !t) {
        return -EINVAL;
    }

    ret = ds3231_read_reg(dev, 0x00, buf, 7);
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
    uint8_t buf[8];
    int ret;

    if (!dev || !t) {
        return -EINVAL;
    }

    buf[0] = 0x00;
    buf[1] = dec_to_bcd(t->second);
    buf[2] = dec_to_bcd(t->minute);
    buf[3] = dec_to_bcd(t->hour);
    buf[4] = t->weekday & 0x07;
    buf[5] = dec_to_bcd(t->day);
    buf[6] = dec_to_bcd(t->month);
    buf[7] = dec_to_bcd(t->year);

    ret = ds3231_write_reg(dev, 0x00, buf + 1, 7);
    if (ret < 0) {
        return ret;
    }

    return 0;
}
