#include "bme280.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_i2c.h"
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_HUM_MSB 0xFD

#define BME280_RESET_WORD 0xB6
#define BME280_CHIP_ID 0x60

#define BME280_TIMEOUT 100

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 2, BME280_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int bme280_read_regs(struct bme280_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), &reg, 1, BME280_TIMEOUT);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), data, len, BME280_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

int bme280_init(struct bme280_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    uint8_t chip_id = 0;
    if (bme280_read_regs(dev, BME280_REG_ID, &chip_id, 1) != 0) return -1;
    if (chip_id != BME280_CHIP_ID) return -1;

    if (bme280_write_reg(dev, BME280_REG_RESET, BME280_RESET_WORD) != 0) return -1;
    HAL_Delay(2);

    if (bme280_write_reg(dev, BME280_REG_CONFIG, 0x00) != 0) return -1;

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t buf[8];
    if (bme280_read_regs(dev, BME280_REG_PRESS_MSB, buf, 8) != 0) return -1;

    uint32_t press = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    press >>= 4;
    *pressure_raw = (int32_t)press;

    uint32_t temp = ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    temp >>= 4;
    *temp_raw = (int32_t)temp;

    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];
    *humidity_raw = (int32_t)hum;

    return 0;
}
