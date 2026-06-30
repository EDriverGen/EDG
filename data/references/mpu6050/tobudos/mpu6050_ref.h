#ifndef __MPU6050_REF_H
#define __MPU6050_REF_H
#include "tos_k.h"
#ifndef HAL_I2C_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#endif
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define MPU6050_ADDR_LOW      0x68
#define MPU6050_ADDR_HIGH     0x69
#define MPU6050_ADDR_DEFAULT  MPU6050_ADDR_LOW
#define MPU6050_WHO_AM_I_VAL  0x68

struct mpu6050_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
};

int mpu6050_init(struct mpu6050_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int mpu6050_probe(struct mpu6050_device *dev);
int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az);
int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz);
#endif
