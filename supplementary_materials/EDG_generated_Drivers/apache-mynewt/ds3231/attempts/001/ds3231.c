#include "ds3231.h"
#include <string.h>

#include "apache_mynewt.h"
static int ds3231_write_then_read(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, uint8_t len) {
    struct hal_i2c_master_data data;
    int rc;

    data.address = DS3231_I2C_ADDR << 1;
    data.buffer = &reg;
    data.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    if (rc != 0) return rc;

    data.buffer = buf;
    data.len = len;
    rc = hal_i2c_master_read(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    return rc;
}

static int ds3231_write_bytes(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, uint8_t len) {
    struct hal_i2c_master_data data;
    uint8_t txbuf[9];
    int rc;

    if (len > 8) return -1;
    txbuf[0] = reg;
    memcpy(&txbuf[1], buf, len);

    data.address = DS3231_I2C_ADDR << 1;
    data.buffer = txbuf;
    data.len = len + 1;
    rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    return rc;
}

static uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(struct ds3231_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t) {
    uint8_t buf[7];
    int rc;

    rc = ds3231_write_then_read(dev, 0x00, buf, 7);
    if (rc != 0) return rc;

    t->second = bcd_to_dec(buf[0] & 0x7F);
    t->minute = bcd_to_dec(buf[1] & 0x7F);
    t->hour = bcd_to_dec(buf[2] & 0x3F);
    t->weekday = buf[3] & 0x07;
    t->day = bcd_to_dec(buf[4] & 0x3F);
    t->month = bcd_to_dec(buf[5] & 0x1F);
    t->year = bcd_to_dec(buf[6]) + 2000;

    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, struct ds3231_time *t) {
    uint8_t buf[7];
    int rc;

    buf[0] = dec_to_bcd(t->second);
    buf[1] = dec_to_bcd(t->minute);
    buf[2] = dec_to_bcd(t->hour);
    buf[3] = t->weekday & 0x07;
    buf[4] = dec_to_bcd(t->day);
    buf[5] = dec_to_bcd(t->month);
    buf[6] = dec_to_bcd(t->year - 2000);

    rc = ds3231_write_bytes(dev, 0x00, buf, 7);
    return rc;
}
