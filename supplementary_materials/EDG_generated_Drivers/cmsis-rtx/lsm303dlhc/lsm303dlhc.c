#include "lsm303dlhc.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#include "stm32f1xx_hal.h"
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

#define STATUS_REG_A 0x27
#define SR_REG_Mg    0x09

#define I2C_TIMEOUT 100

static int i2c_write(I2C_HandleTypeDef *hi2c, uint16_t dev_addr, uint8_t *data, uint16_t size)
{
    if (HAL_I2C_Master_Transmit(hi2c, dev_addr << 1, data, size, I2C_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

static int i2c_write_then_read(I2C_HandleTypeDef *hi2c, uint16_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (HAL_I2C_Mem_Read(hi2c, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    I2C_HandleTypeDef *hi2c = dev->bus_handle;
    uint8_t data[2];

    // Init accelerometer
    data[0] = CTRL_REG1_A; data[1] = 0x57;
    if (i2c_write(hi2c, ACCEL_ADDR, data, 2) != 0) return -1;

    data[0] = CTRL_REG4_A; data[1] = 0x08;
    if (i2c_write(hi2c, ACCEL_ADDR, data, 2) != 0) return -1;

    HAL_Delay(70);

    // Init magnetometer
    data[0] = CRA_REG_M; data[1] = 0x0C;
    if (i2c_write(hi2c, MAG_ADDR, data, 2) != 0) return -1;

    data[0] = CRB_REG_M; data[1] = 0x20;
    if (i2c_write(hi2c, MAG_ADDR, data, 2) != 0) return -1;

    data[0] = MR_REG_M; data[1] = 0x00;
    if (i2c_write(hi2c, MAG_ADDR, data, 2) != 0) return -1;

    HAL_Delay(1);

    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    I2C_HandleTypeDef *hi2c = dev->bus_handle;
    uint8_t status;
    // Poll STATUS_REG_A for ZYXDA
    do {
        if (i2c_write_then_read(hi2c, ACCEL_ADDR, STATUS_REG_A, &status, 1) != 0)
            return -1;
    } while (!(status & 0x08));

    uint8_t buf[6];
    if (i2c_write_then_read(hi2c, ACCEL_ADDR, 0xA8, buf, 6) != 0)
        return -1;

    *ax = (int16_t)((uint16_t)(buf[1] << 8) | buf[0]);
    *ay = (int16_t)((uint16_t)(buf[3] << 8) | buf[2]);
    *az = (int16_t)((uint16_t)(buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz)
{
    I2C_HandleTypeDef *hi2c = dev->bus_handle;
    uint8_t status;
    // Poll SR_REG_Mg for DRDY
    do {
        if (i2c_write_then_read(hi2c, MAG_ADDR, SR_REG_Mg, &status, 1) != 0)
            return -1;
    } while (!(status & 0x01));

    uint8_t buf[6];
    if (i2c_write_then_read(hi2c, MAG_ADDR, OUT_X_H_M, buf, 6) != 0)
        return -1;

    // Order: X_H, X_L, Z_H, Z_L, Y_H, Y_L
    *mx = (int16_t)((uint16_t)(buf[0] << 8) | buf[1]);
    *mz = (int16_t)((uint16_t)(buf[2] << 8) | buf[3]);
    *my = (int16_t)((uint16_t)(buf[4] << 8) | buf[5]);
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t)
{
    I2C_HandleTypeDef *hi2c = dev->bus_handle;
    uint8_t buf[2];
    if (i2c_write_then_read(hi2c, MAG_ADDR, TEMP_OUT_H_M, buf, 2) != 0)
        return -1;

    int16_t raw = (int16_t)((uint16_t)(buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}
