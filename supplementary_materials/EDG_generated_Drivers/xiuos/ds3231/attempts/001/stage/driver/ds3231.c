#include "ds3231.h"
#include "transform.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "bus.h"
#include "bus_pin.h"
#define DS3231_ADDR 0x68
#define REG_SECONDS 0x00
#define REG_MINUTES 0x01
#define REG_HOURS   0x02
#define REG_DAY     0x03
#define REG_DATE    0x04
#define REG_MONTH   0x05
#define REG_YEAR    0x06

static uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(struct ds3231_dev *dev, void *bus_handle) {
    (void)bus_handle;
    /* init_extra_setup_c already opened fd and stored in dev->fd */
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t) {
    uint8_t buf[7];
    uint8_t reg = REG_SECONDS;
    int ret;

    /* Write register pointer */
    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return -EIO;

    /* Read 7 bytes */
    ret = PrivRead(dev->fd, buf, 7);
    if (ret < 0) return -EIO;

    t->second  = bcd_to_dec(buf[0]);
    t->minute  = bcd_to_dec(buf[1]);
    t->hour    = bcd_to_dec(buf[2] & 0x3F); /* 24-hour mode */
    t->weekday = buf[3];
    t->day     = bcd_to_dec(buf[4]);
    t->month   = bcd_to_dec(buf[5] & 0x1F);
    t->year    = bcd_to_dec(buf[6]);

    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t) {
    uint8_t buf[8];
    buf[0] = REG_SECONDS;
    buf[1] = dec_to_bcd(t->second);
    buf[2] = dec_to_bcd(t->minute);
    buf[3] = dec_to_bcd(t->hour);
    buf[4] = t->weekday;
    buf[5] = dec_to_bcd(t->day);
    buf[6] = dec_to_bcd(t->month);
    buf[7] = dec_to_bcd(t->year);

    int ret = PrivWrite(dev->fd, buf, 8);
    if (ret < 0) return -EIO;

    return 0;
}
