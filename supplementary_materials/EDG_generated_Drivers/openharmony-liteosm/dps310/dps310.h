#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>
#include "i2c_if.h"

#define DPS310_I2C_ADDR 0x77

#include "openharmony_liteosm.h"
struct dps310_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int32_t dps310_init(struct dps310_dev *dev, DevHandle bus_handle);
int32_t dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw);
int32_t dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw);

#endif
