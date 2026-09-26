#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>

struct ds3231_dev {
    int fd;
};

struct ds3231_time {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
    uint8_t weekday;
};

int ds3231_init(struct ds3231_dev *dev, const char *bus_name);
int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t);
int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t);

#endif