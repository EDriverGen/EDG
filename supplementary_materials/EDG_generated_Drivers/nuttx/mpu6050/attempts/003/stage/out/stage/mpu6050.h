#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

struct i2c_master_s;

struct mpu6050_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int mpu6050_init(struct mpu6050_dev_s *dev, struct i2c_master_s *bus);
int mpu6050_read_all(struct mpu6050_dev_s *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz);

#endif /* MPU6050_H */