#include "dps310.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_i2c.h"
#define DPS310_REG_PSR_B2 0x00
#define DPS310_REG_TMP_B2 0x03
#define DPS310_REG_COEF   0x10
#define DPS310_REG_MEAS_CFG 0x08
#define DPS310_REG_INT_STS 0x0A
#define DPS310_REG_FIFO_STS 0x0B
#define DPS310_REG_ID     0x0D
#define DPS310_REG_RESET  0x0C

#define DPS310_RESET_PATTERN 0x09

static int dps310_i2c_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int dps310_i2c_write(struct dps310_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 2, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

int dps310_init(struct dps310_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    HAL_Delay(12);
    HAL_Delay(40);

    uint8_t id_buf[1];
    if (dps310_i2c_write_then_read(dev, DPS310_REG_ID, id_buf, 1) != 0) {
        return -1;
    }

    if (dps310_i2c_write(dev, DPS310_REG_RESET, DPS310_RESET_PATTERN) != 0) {
        return -1;
    }

    uint8_t coef_buf[18];
    if (dps310_i2c_write_then_read(dev, DPS310_REG_COEF, coef_buf, 18) != 0) {
        return -1;
    }

    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    if (dev == NULL || pressure_raw == NULL) {
        return -1;
    }
    uint8_t buf[3];
    if (dps310_i2c_write_then_read(dev, DPS310_REG_PSR_B2, buf, 3) != 0) {
        return -1;
    }
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *pressure_raw = raw;
    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    if (dev == NULL || temp_raw == NULL) {
        return -1;
    }
    uint8_t buf[3];
    if (dps310_i2c_write_then_read(dev, DPS310_REG_TMP_B2, buf, 3) != 0) {
        return -1;
    }
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *temp_raw = raw;
    return 0;
}
