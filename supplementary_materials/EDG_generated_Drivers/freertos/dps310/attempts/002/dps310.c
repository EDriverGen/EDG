#include "dps310.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#define DPS310_I2C_ADDR 0x77
#define DPS310_I2C_ADDR_SHIFTED (DPS310_I2C_ADDR << 1)

#define REG_PSR_B2 0x00
#define REG_TMP_B2 0x03
#define REG_COEF   0x10
#define REG_MEAS_CFG 0x08
#define REG_ID     0x0D
#define REG_RESET  0x0C

#define RESET_SOFT_RST 0x09

static int i2c_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, DPS310_I2C_ADDR_SHIFTED, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int i2c_write(struct dps310_dev *dev, uint8_t reg, uint8_t value)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t buf[2] = {reg, value};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, DPS310_I2C_ADDR_SHIFTED, buf, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int dps310_init(struct dps310_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    HAL_Delay(40);

    uint8_t id;
    if (i2c_write_then_read(dev, REG_ID, &id, 1) != 0) return -1;

    if (i2c_write(dev, REG_RESET, RESET_SOFT_RST) != 0) return -1;

    HAL_Delay(12);

    uint8_t coef[18];
    if (i2c_write_then_read(dev, REG_COEF, coef, 18) != 0) return -1;

    return 0;
}

static int32_t read_i24(struct dps310_dev *dev, uint8_t reg)
{
    uint8_t buf[3];
    if (i2c_write_then_read(dev, reg, buf, 3) != 0) return 0;
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    return raw;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    *pressure_raw = read_i24(dev, REG_PSR_B2);
    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    *temp_raw = read_i24(dev, REG_TMP_B2);
    return 0;
}
