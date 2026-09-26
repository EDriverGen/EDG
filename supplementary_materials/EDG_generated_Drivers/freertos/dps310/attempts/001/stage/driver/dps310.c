#include "dps310.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#include "stm32f1xx_hal.h"
#define DPS310_I2C_TIMEOUT 100

static int dps310_read_regs(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, DPS310_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int dps310_write_reg(struct dps310_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), data, 2, DPS310_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

int dps310_init(struct dps310_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    HAL_Delay(40);

    uint8_t id;
    if (dps310_read_regs(dev, 0x0D, &id, 1) != 0) return -1;

    if (dps310_write_reg(dev, 0x0C, 0x09) != 0) return -1;

    HAL_Delay(12);

    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    uint8_t buf[3];
    if (dps310_read_regs(dev, 0x00, buf, 3) != 0) return -1;
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) raw |= 0xFF000000;
    *pressure_raw = raw;
    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    uint8_t buf[3];
    if (dps310_read_regs(dev, 0x03, buf, 3) != 0) return -1;
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) raw |= 0xFF000000;
    *temp_raw = raw;
    return 0;
}
