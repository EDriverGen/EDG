#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <errno.h>
#include "ds3231.h"

#define DS3231_ADDR 0x68

static int ds3231_write_then_read(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_msg_s msg[2];
    int ret;

    msg[0].frequency = 100000;
    msg[0].addr = DS3231_ADDR;
    msg[0].flags = 0;
    msg[0].buffer = &reg;
    msg[0].length = 1;

    msg[1].frequency = 100000;
    msg[1].addr = DS3231_ADDR;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = buf;
    msg[1].length = len;

    ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int ds3231_write_bytes(struct ds3231_dev *dev, uint8_t reg, const uint8_t *data, int len)
{
    uint8_t buffer[8];
    struct i2c_msg_s msg;
    int ret;

    if (len > 7) return -EINVAL;
    buffer[0] = reg;
    for (int i = 0; i < len; i++) {
        buffer[1 + i] = data[i];
    }

    msg.frequency = 100000;
    msg.addr = DS3231_ADDR;
    msg.flags = 0;
    msg.buffer = buffer;
    msg.length = 1 + len;

    ret = I2C_TRANSFER(dev->bus, &msg, 1);
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
    int ret;

    ret = ds3231_write_then_read(dev, 0x00, buf, 7);
    if (ret < 0) {
        return ret;
    }

    t->seconds = ((buf[0] >> 4) * 10) + (buf[0] & 0x0F);
    t->minutes = ((buf[1] >> 4) * 10) + (buf[1] & 0x0F);
    t->hours = ((buf[2] >> 4) * 10) + (buf[2] & 0x0F);
    t->day = buf[3];
    t->date = ((buf[4] >> 4) * 10) + (buf[4] & 0x0F);
    t->month = ((buf[5] >> 4) * 10) + (buf[5] & 0x0F);
    t->year = ((buf[6] >> 4) * 10) + (buf[6] & 0x0F);

    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t)
{
    uint8_t data[7];
    int ret;

    data[0] = ((t->seconds / 10) << 4) | (t->seconds % 10);
    data[1] = ((t->minutes / 10) << 4) | (t->minutes % 10);
    data[2] = ((t->hours / 10) << 4) | (t->hours % 10);
    data[3] = t->day;
    data[4] = ((t->date / 10) << 4) | (t->date % 10);
    data[5] = ((t->month / 10) << 4) | (t->month % 10);
    data[6] = ((t->year / 10) << 4) | (t->year % 10);

    ret = ds3231_write_bytes(dev, 0x00, data, 7);
    if (ret < 0) {
        return ret;
    }
    return 0;
}