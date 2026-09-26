#include "ds3231.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <periph/i2c.h>

#include "riot.h"
#define DS3231_ADDR 0x68

static uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(ds3231_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = DS3231_ADDR;
    i2c_acquire(dev->bus);
    i2c_release(dev->bus);
    return 0;
}

int ds3231_get_time(ds3231_t *dev, ds3231_time *t) {
    uint8_t buf[7];
    int ret;

    i2c_acquire(dev->bus);
    ret = i2c_read_regs(dev->bus, dev->addr, 0x00, buf, 7, 0);
    i2c_release(dev->bus);
    if (ret < 0) {
        return -EIO;
    }

    t->seconds = bcd_to_dec(buf[0]);
    t->minutes = bcd_to_dec(buf[1]);
    t->hours = bcd_to_dec(buf[2]);
    t->day = buf[3];
    t->date = bcd_to_dec(buf[4]);
    t->month = bcd_to_dec(buf[5]);
    t->year = bcd_to_dec(buf[6]);

    return 0;
}

int ds3231_set_time(ds3231_t *dev, const ds3231_time *t) {
    uint8_t buf[8];
    int ret;

    buf[0] = 0x00;
    buf[1] = dec_to_bcd(t->seconds);
    buf[2] = dec_to_bcd(t->minutes);
    buf[3] = dec_to_bcd(t->hours);
    buf[4] = t->day;
    buf[5] = dec_to_bcd(t->date);
    buf[6] = dec_to_bcd(t->month);
    buf[7] = dec_to_bcd(t->year);

    i2c_acquire(dev->bus);
    ret = i2c_write_bytes(dev->bus, dev->addr, buf, 8, 0);
    i2c_release(dev->bus);
    if (ret < 0) {
        return -EIO;
    }

    return 0;
}
