#include "lsm303dlhc.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_i2c.h"
#define ACCEL_ADDR (uint16_t)(LSM303DLHC_ACCEL_ADDR << 1)
#define MAG_ADDR   (uint16_t)(LSM303DLHC_MAG_ADDR << 1)

#define CTRL_REG1_A 0x20
#define CTRL_REG4_A 0x23
#define CRA_REG_M   0x00
#define CRB_REG_M   0x01
#define MR_REG_M    0x02
#define OUT_X_L_A   0x28
#define OUT_X_H_M   0x03
#define TEMP_OUT_H_M 0x31

static int i2c_write(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    if (HAL_I2C_Master_Transmit(hi2c, addr, buf, 2, 100) != HAL_OK)
        return -1;
    return 0;
}

static int i2c_write_then_read(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Mem_Read(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100) != HAL_OK)
        return -1;
    return 0;
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    I2C_HandleTypeDef *hi2c = dev->bus_handle;

    if (i2c_write(hi2c, ACCEL_ADDR, CTRL_REG1_A, 0x57) != 0) return -1;
    if (i2c_write(hi2c, ACCEL_ADDR, CTRL_REG4_A, 0x08) != 0) return -1;
    HAL_Delay(70);

    if (i2c_write(hi2c, MAG_ADDR, CRA_REG_M, 0x0C) != 0) return -1;
    if (i2c_write(hi2c, MAG_ADDR, CRB_REG_M, 0x20) != 0) return -1;
    if (i2c_write(hi2c, MAG_ADDR, MR_REG_M, 0x00) != 0) return -1;
    HAL_Delay(1);

    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    I2C_HandleTypeDef *hi2c = dev->bus_handle;
    uint8_t buf[6];
    if (i2c_write_then_read(hi2c, ACCEL_ADDR, 0xA8, buf, 6) != 0)
        return -1;
    *ax = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
    *ay = (int16_t)((uint16_t)buf[3] << 8 | buf[2]);
    *az = (int16_t)((uint16_t)buf[5] << 8 | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz)
{
    I2C_HandleTypeDef *hi2c = dev->bus_handle;
    uint8_t buf[6];
    if (i2c_write_then_read(hi2c, MAG_ADDR, OUT_X_H_M, buf, 6) != 0)
        return -1;
    *mx = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    *mz = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
    *my = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t)
{
    I2C_HandleTypeDef *hi2c = dev->bus_handle;
    uint8_t buf[2];
    if (i2c_write_then_read(hi2c, MAG_ADDR, TEMP_OUT_H_M, buf, 2) != 0)
        return -1;
    int16_t raw = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}
