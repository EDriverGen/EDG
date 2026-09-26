#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>
#include <stdbool.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} ds3231_t;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} ds3231_time_t;

int ds3231_init(ds3231_t *dev, i2c_t bus);
int ds3231_get_time(ds3231_t *dev, ds3231_time_t *t);
int ds3231_set_time(ds3231_t *dev, const ds3231_time_t *t);

#endif /* DS3231_H */
