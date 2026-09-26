#include <rtthread.h>
#include <stdint.h>
#include <string.h>
#include "ds3231.h"

#define DS3231_ADDR 0x68
#define DS3231_REG_SEC 0x00
#define DS3231_REG_MIN 0x01
#define DS3231_REG_HOUR 0x02
#define DS3231_REG_DAY 0x03
#define DS3231_REG_DATE 0x04
#define DS3231_REG_MONTH 0x05
#define DS3231_REG_YEAR 0x06

static int ds3231_write_regs(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    struct rt_i2c_msg msgs[2];
    uint8_t addr = DS3231_ADDR;
    msgs[0].addr = addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[1].addr = addr;
    msgs[1].flags = RT_I2C_WR | RT_I2C_NO_START;
    msgs[1].buf = buf;
    msgs[1].len = len;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

static int ds3231_read_regs(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    struct rt_i2c_msg msgs[2];
    uint8_t addr = DS3231_ADDR;
    msgs[0].addr = addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[1].addr = addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = buf;
    msgs[1].len = len;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(struct ds3231_device *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    return 0;
}

int ds3231_get_time(struct ds3231_device *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    if (ds3231_read_regs(dev, DS3231_REG_SEC, buf, 7) != 0)
        return -1;
    t->second = bcd_to_dec(buf[0] & 0x7F);
    t->minute = bcd_to_dec(buf[1] & 0x7F);
    t->hour = bcd_to_dec(buf[2] & 0x3F);
    t->weekday = buf[3] & 0x07;
    t->day = bcd_to_dec(buf[4] & 0x3F);
    t->month = bcd_to_dec(buf[5] & 0x1F);
    t->year = bcd_to_dec(buf[6]) + 2000;
    return 0;
}

int ds3231_set_time(struct ds3231_device *dev, const struct ds3231_time *t)
{
    uint8_t buf[7];
    buf[0] = dec_to_bcd(t->second) & 0x7F;
    buf[1] = dec_to_bcd(t->minute) & 0x7F;
    buf[2] = dec_to_bcd(t->hour) & 0x3F;
    buf[3] = t->weekday & 0x07;
    buf[4] = dec_to_bcd(t->day) & 0x3F;
    buf[5] = dec_to_bcd(t->month) & 0x1F;
    buf[6] = dec_to_bcd(t->year - 2000);
    if (ds3231_write_regs(dev, DS3231_REG_SEC, buf, 7) != 0)
        return -1;
    return 0;
}
