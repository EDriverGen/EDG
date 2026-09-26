#include "ds3231.h"
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
#define DS3231_I2C_ADDR 0x68
#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_TEMP_MSB 0x11

static int ds3231_write_then_read(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return -1;
    }
    return 0;
}

static int ds3231_write_bytes(struct ds3231_dev *dev, uint8_t reg, const uint8_t *data, uint16_t len)
{
    uint8_t buf[9];
    if (len > 8) return -1;
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = 1 + len;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

int ds3231_init(struct ds3231_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DS3231_I2C_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    if (ds3231_write_then_read(dev, DS3231_REG_SECONDS, buf, 7) != 0) {
        return -1;
    }
    t->seconds = ((buf[0] >> 4) & 0x07) * 10 + (buf[0] & 0x0F);
    t->minutes = ((buf[1] >> 4) & 0x07) * 10 + (buf[1] & 0x0F);
    t->hours = ((buf[2] >> 4) & 0x03) * 10 + (buf[2] & 0x0F);
    t->day = buf[3] & 0x07;
    t->date = ((buf[4] >> 4) & 0x03) * 10 + (buf[4] & 0x0F);
    t->month = ((buf[5] >> 4) & 0x01) * 10 + (buf[5] & 0x0F);
    t->year = ((buf[6] >> 4) & 0x0F) * 10 + (buf[6] & 0x0F);
    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t)
{
    uint8_t data[7];
    data[0] = ((t->seconds / 10) << 4) | (t->seconds % 10);
    data[1] = ((t->minutes / 10) << 4) | (t->minutes % 10);
    data[2] = ((t->hours / 10) << 4) | (t->hours % 10);
    data[3] = t->day & 0x07;
    data[4] = ((t->date / 10) << 4) | (t->date % 10);
    data[5] = ((t->month / 10) << 4) | (t->month % 10);
    data[6] = ((t->year / 10) << 4) | (t->year % 10);
    return ds3231_write_bytes(dev, DS3231_REG_SECONDS, data, 7);
}
