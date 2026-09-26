#include "ds3231.h"
#include <stddef.h>

#include "apache_mynewt.h"
#define DS3231_ADDR 0x68
#define DS3231_I2C_NUM 0

static uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(struct ds3231_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = DS3231_I2C_NUM;
    dev->addr = DS3231_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t) {
    struct hal_i2c_master_data pdata;
    uint8_t cmd = 0x00;
    uint8_t buf[7];
    int rc;

    pdata.address = dev->addr;
    pdata.buffer = &cmd;
    pdata.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 0);
    if (rc != 0) return -1;

    pdata.buffer = buf;
    pdata.len = 7;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return -1;

    t->second = bcd_to_dec(buf[0] & 0x7F);
    t->minute = bcd_to_dec(buf[1] & 0x7F);
    t->hour = bcd_to_dec(buf[2] & 0x3F);
    t->weekday = buf[3] & 0x07;
    t->day = bcd_to_dec(buf[4] & 0x3F);
    t->month = bcd_to_dec(buf[5] & 0x1F);
    t->year = bcd_to_dec(buf[6]);

    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t) {
    struct hal_i2c_master_data pdata;
    uint8_t buf[8];
    int rc;

    buf[0] = 0x00;
    buf[1] = dec_to_bcd(t->second);
    buf[2] = dec_to_bcd(t->minute);
    buf[3] = dec_to_bcd(t->hour);
    buf[4] = t->weekday & 0x07;
    buf[5] = dec_to_bcd(t->day);
    buf[6] = dec_to_bcd(t->month);
    buf[7] = dec_to_bcd(t->year);

    pdata.address = dev->addr;
    pdata.buffer = buf;
    pdata.len = 8;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return -1;

    return 0;
}
