#include "mpu6050.h"
#include <stdint.h>
#include <string.h>
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"

#include "openharmony_liteosm.h"
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_ACCEL_XOUT_H 0x3B

static int mpu6050_write_reg(struct mpu6050_dev *dev, uint8_t reg, uint8_t data)
{
    struct I2cMsg msgs[1];
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = buf;
    msgs[0].len = 2;
    msgs[0].flags = 0;
    if (I2cTransfer(dev->bus_handle, msgs, 1) != 1) {
        return -1;
    }
    return 0;
}

static int mpu6050_write_then_read(struct mpu6050_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;
    if (I2cTransfer(dev->bus_handle, msgs, 2) != 2) {
        return -1;
    }
    return 0;
}

int mpu6050_init(struct mpu6050_dev *dev, DevHandle bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = MPU6050_I2C_ADDR;

    // Wake up device: clear SLEEP bit in PWR_MGMT_1
    if (mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00) != 0) {
        return -1;
    }

    // Wait 100 ms for stabilization
    OsalMSleep(100);

    // Probe: read WHO_AM_I register
    uint8_t whoami;
    if (mpu6050_write_then_read(dev, MPU6050_WHO_AM_I, &whoami, 1) != 0) {
        return -1;
    }

    return 0;
}

int mpu6050_read_sensors(struct mpu6050_dev *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[14];
    if (mpu6050_write_then_read(dev, MPU6050_ACCEL_XOUT_H, buf, 14) != 0) {
        return -1;
    }

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
    int32_t temp_val = ((int32_t)raw_temp * 1000) / 340 + 36530;
    *temp = temp_val;

    return 0;
}
