#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define LSM303DLHC_ACCEL_ADDR 0x19
#define LSM303DLHC_MAG_ADDR   0x1E

#include "freertos.h"
struct lsm303dlhc_dev {
    I2C_HandleTypeDef *bus_handle;
};

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle);
int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int32_t *ax, int32_t *ay, int32_t *az);
int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int32_t *mx, int32_t *my, int32_t *mz, int32_t *t);

#endif
