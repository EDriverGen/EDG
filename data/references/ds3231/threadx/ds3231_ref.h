#ifndef __DS3231_REF_H
#define __DS3231_REF_H
#include "tx_api.h"
#include <stdint.h>

#define DS3231_ADDR_DEFAULT  0x68

struct ds3231_time {
    uint8_t seconds, minutes, hours;
    uint8_t day, date, month, year;
};


struct ds3231_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct ds3231_device {
  void *bus_context;
  const struct ds3231_i2c_ops *ops;
  uint16_t addr;
};

int ds3231_init(struct ds3231_device *dev, void *bus_context, const struct ds3231_i2c_ops *ops, uint16_t addr);
int ds3231_probe(struct ds3231_device *dev);
int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t);
int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius);
#endif
