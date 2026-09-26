#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "ds3231.h"

#define DS3231_ADDR 0x68

static int ds3231_write_then_read(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = DS3231_ADDR;
    config.addrlen = 7;
    return i2c_writeread(dev->bus, &config, &reg, 1, buf, len);
}

static int ds3231_write(struct ds3231_dev *dev, uint8_t reg, const uint8_t *data, int len)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = DS3231_ADDR;
    config.addrlen = 7;
    uint8_t buf[8];
    buf[0] = reg;
    for (int i = 0; i < len; i++) buf[1+i] = data[i];
    return i2c_write(dev->bus, &config, buf, len+1);
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
    int ret = ds3231_write_then_read(dev, 0x00, buf, 7);
    if (ret < 0) return ret;
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
    uint8_t data[7];
    data[0] = t->seconds;
    data[1] = t->minutes;
    data[2] = t->hours;
    data[3] = t->day;
    data[4] = t->date;
    data[5] = t->month;
    data[6] = t->year;
    return ds3231_write(dev, 0x00, data, 7);
}