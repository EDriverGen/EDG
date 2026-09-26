#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "i2c_if.h"

#define MPU6050_I2C_ADDR 0x68

#include "openharmony_liteosm.h"
struct mpu6050_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int mpu6050_init(struct mpu6050_dev *dev, DevHandle bus_handle);
int mpu6050_read_sensors(struct mpu6050_dev *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz);

#endif
