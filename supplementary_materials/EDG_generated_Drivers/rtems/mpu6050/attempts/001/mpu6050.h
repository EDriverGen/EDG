#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

struct mpu6050_device {
    int fd;
    uint8_t i2c_addr;
};

int mpu6050_init(struct mpu6050_device *dev, void *bus_handle);
int mpu6050_read_all(struct mpu6050_device *dev, int32_t *ax, int32_t *ay, int32_t *az, int32_t *temp, int32_t *gx, int32_t *gy, int32_t *gz);

#endif /* MPU6050_H */