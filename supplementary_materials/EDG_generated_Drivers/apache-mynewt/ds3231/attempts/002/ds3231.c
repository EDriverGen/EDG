#include "ds3231.h"
#include <stddef.h>

#include "apache_mynewt.h"
#define DS3231_ADDR 0x68
#define TIMEOUT_MS 1000

static int bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(int dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(struct ds3231_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->i2c_num = (uintptr_t)bus_handle;
    dev->addr = DS3231_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t) {
    if (!dev || !t) return -1;
    uint8_t reg = 0x00;
    uint8_t buf[7];
    struct hal_i2c_master_data pdata;
    pdata.address = dev->addr << 1;
    pdata.buffer = &reg;
    pdata.len = 1;
    int rc = hal_i2c_master_write(dev->i2c_num, &pdata, TIMEOUT_MS, 1);
    if (rc) return rc;
    pdata.buffer = buf;
    pdata.len = 7;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, TIMEOUT_MS, 1);
    if (rc) return rc;
    t->second = bcd_to_dec(buf[0]);
    t->minute = bcd_to_dec(buf[1]);
    t->hour = bcd_to_dec(buf[2] & 0x3F);
    t->weekday = buf[3];
    t->day = bcd_to_dec(buf[4]);
    t->month = bcd_to_dec(buf[5] & 0x1F);
    t->year = bcd_to_dec(buf[6]) + 2000;
    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, struct ds3231_time *t) {
    if (!dev || !t) return -1;
    uint8_t buf[8];
    buf[0] = 0x00;
    buf[1] = dec_to_bcd(t->second);
    buf[2] = dec_to_bcd(t->minute);
    buf[3] = dec_to_bcd(t->hour);
    buf[4] = t->weekday;
    buf[5] = dec_to_bcd(t->day);
    buf[6] = dec_to_bcd(t->month);
    buf[7] = dec_to_bcd(t->year - 2000);
    struct hal_i2c_master_data pdata;
    pdata.address = dev->addr << 1;
    pdata.buffer = buf;
    pdata.len = 8;
    return hal_i2c_master_write(dev->i2c_num, &pdata, TIMEOUT_MS, 1);
}
