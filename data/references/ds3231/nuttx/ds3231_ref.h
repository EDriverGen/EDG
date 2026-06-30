#ifndef __DS3231_REF_H
#define __DS3231_REF_H
#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#define DS3231_ADDR_DEFAULT  0x68

struct ds3231_time {
    uint8_t seconds, minutes, hours;
    uint8_t day, date, month, year;
};

struct ds3231_device {
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
};

int ds3231_init(struct ds3231_device *dev, FAR struct i2c_master_s *i2c, uint16_t addr);
int ds3231_probe(struct ds3231_device *dev);
int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t);
int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius);
#endif
