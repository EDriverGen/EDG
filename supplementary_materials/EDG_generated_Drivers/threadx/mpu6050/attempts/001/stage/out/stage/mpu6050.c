#include "mpu6050.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "threadx.h"
#define MPU6050_I2C_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_ACCEL_XOUT_H 0x3B

static int mpu6050_write_reg(struct mpu6050_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(MPU6050_I2C_ADDR << 1), buf, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int mpu6050_read_regs(struct mpu6050_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(MPU6050_I2C_ADDR << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int mpu6050_init(struct mpu6050_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = MPU6050_I2C_ADDR;

    // Wake up device: clear SLEEP bit in PWR_MGMT_1
    if (mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00) != 0)
        return -1;

    HAL_Delay(100);

    // Probe: read WHO_AM_I
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

    // Temperature conversion: ((raw * 1000) / 340) + 36530
    int64_t temp_val = (int64_t)raw_temp * 1000 / 340 + 36530;
    *temp = (int32_t)temp_val;

    return 0;
}
