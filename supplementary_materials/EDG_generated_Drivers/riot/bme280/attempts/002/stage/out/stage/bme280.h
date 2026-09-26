#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} bme280_t;

int bme280_init(bme280_t *dev, i2c_t bus);
int bme280_read_all(bme280_t *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw);

#endif
