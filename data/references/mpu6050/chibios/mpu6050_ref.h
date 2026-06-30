#ifndef __MPU6050_REF_H
#define __MPU6050_REF_H
#include "hal.h"
#include <stdint.h>

#define MPU6050_ADDR_LOW      0x68
#define MPU6050_ADDR_HIGH     0x69
#define MPU6050_ADDR_DEFAULT  MPU6050_ADDR_LOW
#define MPU6050_WHO_AM_I_VAL  0x68

struct mpu6050_device {
  I2CDriver * bus;
  uint16_t addr;
};

int mpu6050_init(struct mpu6050_device *dev, I2CDriver * bus, uint16_t addr);
int mpu6050_probe(struct mpu6050_device *dev);
int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az);
int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz);
#endif

