#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
struct device;

int mpu6050_init(const struct device *dev);
int mpu6050_read_all(const struct device *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz);

#endif /* MPU6050_H */
