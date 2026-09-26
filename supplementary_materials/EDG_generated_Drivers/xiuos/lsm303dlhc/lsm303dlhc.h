#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>
#include "bus_i2c.h"

#define LSM303DLHC_ACCEL_ADDR 0x19
#define LSM303DLHC_MAG_ADDR   0x1E

struct I2cBus;

struct lsm303dlhc_dev {
    struct I2cBus *bus;
    int fd;
};

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, struct I2cBus *bus_handle);
int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az);
int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz);
int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t);

#endif