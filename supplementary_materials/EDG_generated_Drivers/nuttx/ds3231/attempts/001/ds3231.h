#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>

struct i2c_master_s;

struct ds3231_dev {
    struct i2c_master_s *bus;
    uint8_t addr;
};

struct ds3231_time {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
};

int ds3231_init(struct ds3231_dev *dev, struct i2c_master_s *bus);
int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t);
int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t);

#endif /* DS3231_H */