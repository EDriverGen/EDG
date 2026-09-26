#include "mpu6050.h"
#include "hal.h"
#include "hal_i2c.h"
#include <string.h>

#define MPU6050_I2C_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_TEMP_OUT_H 0x41

static int mpu6050_write_reg(struct mpu6050_dev *dev, uint8_t reg, uint8_t data)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t txbuf[2] = {reg, data};
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

static int mpu6050_read_regs(struct mpu6050_dev *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, &reg, 1, buf, len, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

int mpu6050_init(struct mpu6050_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = MPU6050_I2C_ADDR;

    // Wake up device: clear SLEEP bit in PWR_MGMT_1
    if (mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00) != 0)
        return -1;

    // Wait 100 ms for stabilization
    chThdSleepMilliseconds(100);

    // Probe: read WHO_AM_I register
    uint8_t whoami;
    if (mpu6050_read_regs(dev, MPU6050_WHO_AM_I, &whoami, 1) != 0)
        return -1;

    return 0;
}

int mpu6050_read_all(struct mpu6050_dev *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[14];
    if (mpu6050_read_regs(dev, MPU6050_ACCEL_XOUT_H, buf, 14) != 0)
        return -1;

    // Parse big-endian 16-bit values
    int16_t raw_ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t raw_temp = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t raw_gx = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t raw_gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t raw_gz = (int16_t)((buf[12] << 8) | buf[13]);

    // Assign raw values for accel and gyro (raw_count semantics)
    *ax = raw_ax;
    *ay = raw_ay;
    *az = raw_az;
    *gx = raw_gx;
    *gy = raw_gy;
    *gz = raw_gz;

    // Temperature conversion: ((raw * 1000) // 340) + 36530
    int32_t temp_raw = (int32_t)raw_temp;
    *temp = (temp_raw * 1000) / 340 + 36530;

    return 0;
}