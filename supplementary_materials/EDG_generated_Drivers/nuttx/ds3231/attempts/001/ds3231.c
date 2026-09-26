#include "ds3231.h"
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#define DS3231_ADDR 0x68
#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_TEMP_MSB 0x11

static int ds3231_write_then_read(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = DS3231_ADDR;
    config.addrlen = 7;
    int ret = I2C_WRITEREAD(dev->bus, &config, &reg, 1, buf, len);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int ds3231_write_bytes(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = DS3231_ADDR;
    config.addrlen = 7;
    uint8_t data[8];
    data[0] = reg;
    for (int i = 0; i < len; i++) {
        data[i+1] = buf[i];
    }
    int ret = I2C_WRITE(dev->bus, &config, data, len + 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int ds3231_init(struct ds3231_dev *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = DS3231_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    int ret = ds3231_write_then_read(dev, DS3231_REG_SECONDS, buf, 7);
    if (ret < 0) {
        return ret;
    }
    t->seconds = buf[0];
    t->minutes = buf[1];
    t->hours = buf[2];
    t->day = buf[3];
    t->date = buf[4];
    t->month = buf[5];
    t->year = buf[6];
    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t)
{
    uint8_t buf[7];
    buf[0] = t->seconds;
    buf[1] = t->minutes;
    buf[2] = t->hours;
    buf[3] = t->day;
    buf[4] = t->date;
    buf[5] = t->month;
    buf[6] = t->year;
    int ret = ds3231_write_bytes(dev, DS3231_REG_SECONDS, buf, 7);
    if (ret < 0) {
        return ret;
    }
    return 0;
}