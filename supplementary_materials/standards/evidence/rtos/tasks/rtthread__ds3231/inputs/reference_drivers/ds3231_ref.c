/*
 * DS3231 RTC Driver
 * Registers 0x00-0x06: BCD time, 0x11-0x12: temperature (10-bit, 0.25C resolution)
 */
#include "ds3231_ref.h"


static int ds3231_read_reg(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    struct rt_i2c_msg msgs[2];
    msgs[0].addr = dev->addr; msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg; msgs[0].len = 1;
    msgs[1].addr = dev->addr; msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = buf; msgs[1].len = len;
    return (rt_i2c_transfer(dev->bus, msgs, 2) == 2) ? 0 : -1;
}
static int ds3231_write_reg(struct ds3231_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    struct rt_i2c_msg msg;
    msg.addr = dev->addr; msg.flags = RT_I2C_WR;
    msg.buf = buf; msg.len = 2;
    return (rt_i2c_transfer(dev->bus, &msg, 1) == 1) ? 0 : -1;
}

static uint8_t bcd_to_dec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }

int ds3231_init(struct ds3231_device *dev, struct rt_i2c_bus_device * bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int ds3231_probe(struct ds3231_device *dev) {
    uint8_t val;
    return ds3231_read_reg(dev, 0x00, &val, 1);
}

int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t) {
    uint8_t buf[7]; int ret;
    if (!dev || !t) return -1;
    ret = ds3231_read_reg(dev, 0x00, buf, 7);
    if (ret) return ret;
    t->seconds = bcd_to_dec(buf[0] & 0x7F);
    t->minutes = bcd_to_dec(buf[1] & 0x7F);
    t->hours   = bcd_to_dec(buf[2] & 0x3F);
    t->day     = bcd_to_dec(buf[3] & 0x07);
    t->date    = bcd_to_dec(buf[4] & 0x3F);
    t->month   = bcd_to_dec(buf[5] & 0x1F);
    t->year    = bcd_to_dec(buf[6]);
    return 0;
}

int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius) {
    uint8_t buf[2]; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = ds3231_read_reg(dev, 0x11, buf, 2);
    if (ret) return ret;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *temp_mcelsius = ((int32_t)(raw >> 6) * 250);
    return 0;
}
