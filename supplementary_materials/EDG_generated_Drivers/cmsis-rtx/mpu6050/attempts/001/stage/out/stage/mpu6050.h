#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "cmsis_rtx.h"
struct mpu6050_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int mpu6050_init(struct mpu6050_dev *dev, void *bus_handle);
int mpu6050_read_all(struct mpu6050_dev *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz);

#endif
