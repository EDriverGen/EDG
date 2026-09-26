#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>

#include "apache_mynewt.h"
struct ds3231_dev {
    uint8_t i2c_num;
    uint8_t addr;
};

struct ds3231_time {
    uint8_t year;      /* 0..99 */
    uint8_t month;     /* 1..12 */
    uint8_t day;       /* 1..31 */
    uint8_t hour;      /* 0..23 */
    uint8_t minute;    /* 0..59 */
    uint8_t second;    /* 0..59 */
    uint8_t weekday;   /* 1..7 */
};

int ds3231_init(struct ds3231_dev *dev, void *bus_handle);
int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t);
int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t);

#endif /* DS3231_H */
