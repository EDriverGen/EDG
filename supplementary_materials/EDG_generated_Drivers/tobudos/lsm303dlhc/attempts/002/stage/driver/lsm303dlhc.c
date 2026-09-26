#include "lsm303dlhc.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_i2c.h"
#define I2C_TIMEOUT 100

static int lsm303dlhc_i2c_write(struct lsm303dlhc_dev *dev, uint16_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, addr << 1, buf, 2, I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int lsm303dlhc_i2c_read(struct lsm303dlhc_dev *dev, uint16_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(dev->bus_handle, addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;

    // Init accelerometer
    if (lsm303dlhc_i2c_write(dev, LSM303DLHC_ACCEL_ADDR, 0x20, 0x57) != 0) return -1;
    if (lsm303dlhc_i2c_write(dev, LSM303DLHC_ACCEL_ADDR, 0x23, 0x08) != 0) return -1;
    HAL_Delay(70);

    // Init magnetometer
    if (lsm303dlhc_i2c_write(dev, LSM303DLHC_MAG_ADDR, 0x00, 0x0C) != 0) return -1;
    if (lsm303dlhc_i2c_write(dev, LSM303DLHC_MAG_ADDR, 0x01, 0x20) != 0) return -1;
    if (lsm303dlhc_i2c_write(dev, LSM303DLHC_MAG_ADDR, 0x02, 0x00) != 0) return -1;
    HAL_Delay(1);

    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    if (lsm303dlhc_i2c_read(dev, LSM303DLHC_ACCEL_ADDR, 0xA8, buf, 6) != 0) return -1;
    *ax = (int16_t)((buf[1] << 8) | buf[0]);
    *ay = (int16_t)((buf[3] << 8) | buf[2]);
    *az = (int16_t)((buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz)
{
    uint8_t buf[6];
    if (lsm303dlhc_i2c_read(dev, LSM303DLHC_MAG_ADDR, 0x03, buf, 6) != 0) return -1;
    *mx = (int16_t)((buf[0] << 8) | buf[1]);
    *mz = (int16_t)((buf[2] << 8) | buf[3]);
    *my = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t)
{
    uint8_t buf[2];
    if (lsm303dlhc_i2c_read(dev, LSM303DLHC_MAG_ADDR, 0x31, buf, 2) != 0) return -1;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}
