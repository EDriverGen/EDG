#ifndef __MPU6050_REF_H
#define __MPU6050_REF_H
#include "tx_api.h"
#include <stdint.h>

#define MPU6050_ADDR_LOW      0x68
#define MPU6050_ADDR_HIGH     0x69
#define MPU6050_ADDR_DEFAULT  MPU6050_ADDR_LOW
#define MPU6050_WHO_AM_I_VAL  0x68


struct mpu6050_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct mpu6050_device {
  void *bus_context;
  const struct mpu6050_i2c_ops *ops;
  uint16_t addr;
};

int mpu6050_init(struct mpu6050_device *dev, void *bus_context, const struct mpu6050_i2c_ops *ops, uint16_t addr);
int mpu6050_probe(struct mpu6050_device *dev);
int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az);
int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz);
#endif
