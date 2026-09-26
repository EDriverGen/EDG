#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>
#include <zephyr/device.h>

#include <zephyr/drivers/i2c.h>
struct device;

struct ds3231_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint8_t year;
};

int ds3231_init(const struct device *dev);
int ds3231_get_time(const struct device *dev, struct ds3231_time *t);
int ds3231_set_time(const struct device *dev, const struct ds3231_time *t);

#endif /* DS3231_H */
