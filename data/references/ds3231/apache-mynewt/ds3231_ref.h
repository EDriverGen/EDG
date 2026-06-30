#ifndef DS3231_APACHE_MYNEWT_REF_H
#define DS3231_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_i2c.h"
#include <stdint.h>

#define DS3231_ADDR_DEFAULT 0x68U

struct ds3231_time {
    uint8_t seconds, minutes, hours;
    uint8_t day, date, month, year;
};

struct ds3231_device {
    uint8_t i2c_num;
    uint16_t addr;
};

int ds3231_init(struct ds3231_device *dev, uint8_t i2c_num, uint16_t addr);
int ds3231_probe(struct ds3231_device *dev);
int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t);
int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius);

#endif
