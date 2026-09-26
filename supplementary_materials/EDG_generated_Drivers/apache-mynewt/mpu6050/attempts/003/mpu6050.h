#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

#define MPU6050_I2C_ADDR 0x68

#include <hal/hal_i2c.h>
struct mpu6050_dev {
    uint8_t i2c_num;
    uint8_t i2c_addr;
};

int mpu6050_init(struct mpu6050_dev *dev, void *bus_handle);
int mpu6050_read_all(struct mpu6050_dev *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz);

#endif
