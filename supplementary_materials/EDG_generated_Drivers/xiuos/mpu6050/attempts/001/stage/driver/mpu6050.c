#include "mpu6050.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I   0x75
#define MPU6050_ACCEL_XOUT_H 0x3B

static int mpu6050_write_reg(struct mpu6050_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    int ret = PrivWrite(dev->fd, buf, 2);
    if (ret < 0) return -EIO;
    return 0;
}

static int mpu6050_write_then_read(struct mpu6050_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return -EIO;
    ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) return -EIO;
    return 0;
}

int mpu6050_init(struct mpu6050_dev *dev, struct I2cBus *bus_handle)
{
    dev->bus = bus_handle;
    dev->i2c_addr = MPU6050_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -EIO;

    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) { PrivClose(dev->fd); return -EIO; }

    ret = mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    PrivTaskDelay(100);

    uint8_t whoami;
    ret = mpu6050_write_then_read(dev, MPU6050_WHO_AM_I, &whoami, 1);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    return 0;
}

int mpu6050_read_all(struct mpu6050_dev *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz)
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
