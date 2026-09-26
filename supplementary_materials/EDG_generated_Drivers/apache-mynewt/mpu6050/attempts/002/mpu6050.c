#include "mpu6050.h"
#include <string.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_ACCEL_XOUT_H 0x3B

static int mpu6050_write_reg(struct mpu6050_dev *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data data;
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    data.address = dev->i2c_addr;
    data.buffer = buf;
    data.len = 2;
    return hal_i2c_master_write(dev->i2c_num, &data, OS_TICKS_PER_SEC / 10, 1);
}

static int mpu6050_write_then_read(struct mpu6050_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data data;
    int rc;
    data.address = dev->i2c_addr;
    data.buffer = &reg;
    data.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TICKS_PER_SEC / 10, 0);
    if (rc != 0) return rc;
    data.buffer = buf;
    data.len = len;
    return hal_i2c_master_read(dev->i2c_num, &data, OS_TICKS_PER_SEC / 10, 1);
}

int mpu6050_init(struct mpu6050_dev *dev, void *bus_handle)
{
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->i2c_addr = MPU6050_I2C_ADDR;
    int rc;
    rc = mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00);
    if (rc != 0) return rc;
    os_time_delay(OS_TICKS_PER_SEC / 10);
    uint8_t whoami;
    rc = mpu6050_write_then_read(dev, MPU6050_WHO_AM_I, &whoami, 1);
    if (rc != 0) return rc;
    if (whoami != 0x68) return -1;
    return 0;
}

int mpu6050_read_all(struct mpu6050_dev *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[14];
    int rc = mpu6050_write_then_read(dev, MPU6050_ACCEL_XOUT_H, buf, 14);
    if (rc != 0) return rc;
    *ax = (int16_t)((buf[0] << 8) | buf[1]);
    *ay = (int16_t)((buf[2] << 8) | buf[3]);
    *az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t raw_temp = (int16_t)((buf[6] << 8) | buf[7]);
    *temp = ((int32_t)raw_temp * 1000) / 340 + 36530;
    *gx = (int16_t)((buf[8] << 8) | buf[9]);
    *gy = (int16_t)((buf[10] << 8) | buf[11]);
    *gz = (int16_t)((buf[12] << 8) | buf[13]);
    return 0;
}
