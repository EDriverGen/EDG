#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} mpu6050_t;

int mpu6050_init(mpu6050_t *dev, i2c_t bus);
int mpu6050_read_all(mpu6050_t *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz);

#endif
