#include "mpu6050.h"
#include <stddef.h>
#include <errno.h>
#include "xtimer.h"
#include "periph/i2c.h"

#include "riot.h"
#define MPU6050_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_WHO_AM_I 0x75

int mpu6050_init(mpu6050_t *dev, i2c_t bus)
{
    dev->bus = bus;
    dev->addr = MPU6050_ADDR;

    uint8_t cmd[] = {0x6B, 0x00};
    int ret = i2c_write_bytes(bus, dev->addr, cmd, 2, 0);
    if (ret < 0) {
        return -EIO;
    }

    xtimer_msleep(100);

    uint8_t who_am_i;
    ret = i2c_read_regs(bus, dev->addr, MPU6050_WHO_AM_I, &who_am_i, 1, 0);
    if (ret < 0) {
        return -EIO;
    }

    return 0;
}

int mpu6050_read_all(mpu6050_t *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[14];
    int ret = i2c_read_regs(dev->bus, dev->addr, MPU6050_ACCEL_XOUT_H, buf, 14, 0);
    if (ret < 0) {
        return -EIO;
    }

    *ax = (int16_t)((buf[0] << 8) | buf[1]);
    *ay = (int16_t)((buf[2] << 8) | buf[3]);
    *az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t raw_temp = (int16_t)((buf[6] << 8) | buf[7]);
    *gx = (int16_t)((buf[8] << 8) | buf[9]);
    *gy = (int16_t)((buf[10] << 8) | buf[11]);
    *gz = (int16_t)((buf[12] << 8) | buf[13]);

    *temp = ((int32_t)raw_temp * 1000) / 340 + 36530;

    return 0;
}
