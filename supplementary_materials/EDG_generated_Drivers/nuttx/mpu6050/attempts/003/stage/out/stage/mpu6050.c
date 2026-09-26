#include "mpu6050.h"
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "arch.h"

#define MPU6050_I2C_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_ACCEL_XOUT_H 0x3B

static int mpu6050_write_reg(struct mpu6050_dev_s *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg_s msg[2];
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    msg[0].frequency = 100000;
    msg[0].addr = MPU6050_I2C_ADDR;
    msg[0].flags = 0;
    msg[0].buffer = buf;
    msg[0].length = 2;
    return I2C_TRANSFER(dev->bus, msg, 1);
}

static int mpu6050_write_then_read(struct mpu6050_dev_s *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_msg_s msg[2];
    msg[0].frequency = 100000;
    msg[0].addr = MPU6050_I2C_ADDR;
    msg[0].flags = 0;
    msg[0].buffer = &reg;
    msg[0].length = 1;
    msg[1].frequency = 100000;
    msg[1].addr = MPU6050_I2C_ADDR;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = buf;
    msg[1].length = len;
    return I2C_TRANSFER(dev->bus, msg, 2);
}

int mpu6050_init(struct mpu6050_dev_s *dev, struct i2c_master_s *bus)
{
    int ret;
    dev->bus = bus;
    dev->addr = MPU6050_I2C_ADDR;
    ret = mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00);
    if (ret < 0) return ret;
    up_mdelay(100);
    uint8_t whoami;
    ret = mpu6050_write_then_read(dev, MPU6050_WHO_AM_I, &whoami, 1);
    if (ret < 0) return ret;
    return 0;
}

int mpu6050_read_all(struct mpu6050_dev_s *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[14];
    int ret = mpu6050_write_then_read(dev, MPU6050_ACCEL_XOUT_H, buf, 14);
    if (ret < 0) return ret;
    int16_t raw_ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t raw_temp = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t raw_gx = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t raw_gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t raw_gz = (int16_t)((buf[12] << 8) | buf[13]);
    *ax = raw_ax;
    *ay = raw_ay;
    *az = raw_az;
    *gx = raw_gx;
    *gy = raw_gy;
    *gz = raw_gz;
    *temp = ((int32_t)raw_temp * 1000) / 340 + 36530;
    return 0;
}