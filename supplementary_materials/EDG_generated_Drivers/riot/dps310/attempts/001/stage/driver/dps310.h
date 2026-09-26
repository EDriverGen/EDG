#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} dps310_t;

int dps310_init(dps310_t *dev, i2c_t bus);
int dps310_read_pressure(dps310_t *dev, int32_t *pressure_raw);
int dps310_read_temp(dps310_t *dev, int32_t *temp_raw);

#endif /* DPS310_H */
