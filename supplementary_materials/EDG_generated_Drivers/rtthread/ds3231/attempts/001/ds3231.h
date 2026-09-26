#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>

struct ds3231_device {
    struct rt_i2c_bus_device *bus;
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

int ds3231_init(struct ds3231_device *dev, struct rt_i2c_bus_device *bus);
int ds3231_get_time(struct ds3231_device *dev, struct ds3231_time *t);
int ds3231_set_time(struct ds3231_device *dev, const struct ds3231_time *t);

#endif /* DS3231_H */