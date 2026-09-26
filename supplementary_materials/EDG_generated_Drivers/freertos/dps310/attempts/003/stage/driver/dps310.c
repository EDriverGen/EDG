#include "dps310.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#include "stm32f1xx_hal.h"
#define DPS310_I2C_TIMEOUT 100

static int dps310_write_then_read(struct dps310_dev *dev, uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(dev->bus_handle, (dev->i2c_addr << 1), mem_addr, I2C_MEMADD_SIZE_8BIT, data, len, DPS310_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int dps310_write(struct dps310_dev *dev, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (dev->i2c_addr << 1), data, len, DPS310_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int32_t dps310_read_i24(uint8_t *buf)
{
    uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    return (int32_t)raw;
}

int dps310_init(struct dps310_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    HAL_Delay(40);

    uint8_t id_reg = 0x0D;
    uint8_t id_val = 0;
    if (dps310_write_then_read(dev, id_reg, &id_val, 1) != 0) return -1;

    uint8_t reset_cmd[2] = {0x0C, 0x09};
    if (dps310_write(dev, reset_cmd, 2) != 0) return -1;

    HAL_Delay(12);

    uint8_t coef_reg = 0x10;
    uint8_t coef_buf[18];
    if (dps310_write_then_read(dev, coef_reg, coef_buf, 18) != 0) return -1;

    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    uint8_t reg = 0x00;
    uint8_t buf[3];
    if (dps310_write_then_read(dev, reg, buf, 3) != 0) return -1;
    *pressure_raw = dps310_read_i24(buf);
    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    uint8_t reg = 0x03;
    uint8_t buf[3];
    if (dps310_write_then_read(dev, reg, buf, 3) != 0) return -1;
    *temp_raw = dps310_read_i24(buf);
    return 0;
}
