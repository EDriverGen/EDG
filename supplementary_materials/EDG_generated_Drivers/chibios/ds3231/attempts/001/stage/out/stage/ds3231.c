#include "ds3231.h"
#include "ch.h"
#include "hal.h"
#include <stdint.h>
#include <string.h>

#include "chibios.h"
#define DS3231_ADDR 0x68
#define DS3231_TIMEOUT MS2ST(100)

static int ds3231_write_then_read(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, size_t len) {
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, DS3231_ADDR, &reg, 1, buf, len, DS3231_TIMEOUT);
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int ds3231_write(struct ds3231_device *dev, uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t txbuf[1 + len];
    txbuf[0] = reg;
    memcpy(&txbuf[1], data, len);
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, DS3231_ADDR, txbuf, 1 + len, NULL, 0, DS3231_TIMEOUT);
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

int ds3231_init(struct ds3231_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DS3231_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_device *dev, struct ds3231_time *t) {
    uint8_t buf[7];
    if (ds3231_write_then_read(dev, 0x00, buf, 7) != 0) {
        return -1;
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

int ds3231_set_time(struct ds3231_device *dev, const struct ds3231_time *t) {
    uint8_t data[7];
    data[0] = ((t->seconds / 10) << 4) | (t->seconds % 10);
    data[1] = ((t->minutes / 10) << 4) | (t->minutes % 10);
    data[2] = ((t->hours / 10) << 4) | (t->hours % 10);
    data[3] = t->day;
    data[4] = ((t->date / 10) << 4) | (t->date % 10);
    data[5] = ((t->month / 10) << 4) | (t->month % 10);
    data[6] = ((t->year / 10) << 4) | (t->year % 10);
    return ds3231_write(dev, 0x00, data, 7);
}
