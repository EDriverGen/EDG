#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>

#include "apache_mynewt.h"
struct ds3231_dev {
    uint8_t i2c_num;
    uint8_t addr;
};

struct ds3231_time {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
};

int ds3231_init(struct ds3231_dev *dev, void *bus_handle);
int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t);
int ds3231_set_time(struct ds3231_dev *dev, struct ds3231_time *t);

#endif
