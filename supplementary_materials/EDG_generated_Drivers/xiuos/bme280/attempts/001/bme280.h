#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "bus_i2c.h"

struct I2cBus;

struct bme280_dev {
    struct I2cBus *bus;
    int fd;
    uint8_t i2c_addr;
};

int bme280_init(struct bme280_dev *dev, struct I2cBus *bus_handle);
int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw);

#endif