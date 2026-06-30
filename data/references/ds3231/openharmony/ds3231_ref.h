#ifndef __DS3231_REF_H
#define __DS3231_REF_H
#include "i2c_if.h"
#include "osal_time.h"
#include <stdint.h>

#define DS3231_ADDR_DEFAULT  0x68

struct ds3231_time {
    uint8_t seconds, minutes, hours;
    uint8_t day, date, month, year;
};

struct ds3231_device {
    DevHandle bus;
    uint16_t addr;
};

int ds3231_init(struct ds3231_device *dev, DevHandle bus, uint16_t addr);
int ds3231_probe(struct ds3231_device *dev);
int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t);
int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius);
#endif
