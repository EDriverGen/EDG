#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>
#include "bus_i2c.h"

struct I2cBus;

struct dps310_dev {
    struct I2cBus *bus;
    int fd;
    uint8_t i2c_addr;
};

int dps310_init(struct dps310_dev *dev, struct I2cBus *bus_handle);
int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw);
int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw);

#endif