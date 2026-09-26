#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
} lsm303dlhc_t;

int lsm303dlhc_init(lsm303dlhc_t *dev, i2c_t bus);
int lsm303dlhc_read_accel(lsm303dlhc_t *dev, int32_t *ax, int32_t *ay, int32_t *az);
int lsm303dlhc_read_mag(lsm303dlhc_t *dev, int32_t *mx, int32_t *my, int32_t *mz, int32_t *t);

#endif /* LSM303DLHC_H */
