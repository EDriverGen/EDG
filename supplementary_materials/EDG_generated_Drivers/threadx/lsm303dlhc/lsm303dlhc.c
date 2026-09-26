#include "lsm303dlhc.h"
#include <stddef.h>
#include "stm32f1xx_hal.h"

#include "threadx.h"
#define ACCEL_ADDR 0x19
#define MAG_ADDR   0x1E

#define CTRL_REG1_A 0x20
#define CTRL_REG4_A 0x23
#define CRA_REG_M   0x00
#define CRB_REG_M   0x01
#define MR_REG_M    0x02
#define OUT_X_L_A   0x28
#define OUT_X_H_M   0x03
#define TEMP_OUT_H_M 0x31

static int i2c_write(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Master_Transmit(hi2c, dev_addr << 1, data, len, 100) != HAL_OK)
        return -1;
    return 0;
}

static int i2c_read(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (HAL_I2C_Mem_Read(hi2c, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) != HAL_OK)
        return -1;
    return 0;
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->accel_addr = ACCEL_ADDR;
    dev->mag_addr = MAG_ADDR;

    uint8_t buf[2];

    // Init accelerometer
    buf[0] = CTRL_REG1_A;
    buf[1] = 0x57;
    if (i2c_write(dev->bus_handle, dev->accel_addr, buf, 2) != 0)
        return -1;

    buf[0] = CTRL_REG4_A;
    buf[1] = 0x08;
    if (i2c_write(dev->bus_handle, dev->accel_addr, buf, 2) != 0)
        return -1;

    HAL_Delay(70);

    // Init magnetometer
    buf[0] = CRA_REG_M;
    buf[1] = 0x0C;
    if (i2c_write(dev->bus_handle, dev->mag_addr, buf, 2) != 0)
        return -1;

    buf[0] = CRB_REG_M;
    buf[1] = 0x20;
    if (i2c_write(dev->bus_handle, dev->mag_addr, buf, 2) != 0)
        return -1;

    buf[0] = MR_REG_M;
    buf[1] = 0x00;
    if (i2c_write(dev->bus_handle, dev->mag_addr, buf, 2) != 0)
        return -1;

    HAL_Delay(1);

    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    if (i2c_read(dev->bus_handle, dev->accel_addr, 0xA8, buf, 6) != 0)
        return -1;

    *ax = (int16_t)((buf[1] << 8) | buf[0]);
    *ay = (int16_t)((buf[3] << 8) | buf[2]);
    *az = (int16_t)((buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz)
{
    uint8_t buf[6];
    if (i2c_read(dev->bus_handle, dev->mag_addr, OUT_X_H_M, buf, 6) != 0)
        return -1;

    *mx = (int16_t)((buf[0] << 8) | buf[1]);
    *mz = (int16_t)((buf[2] << 8) | buf[3]);
    *my = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t)
{
    uint8_t buf[2];
    if (i2c_read(dev->bus_handle, dev->mag_addr, TEMP_OUT_H_M, buf, 2) != 0)
        return -1;

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}
