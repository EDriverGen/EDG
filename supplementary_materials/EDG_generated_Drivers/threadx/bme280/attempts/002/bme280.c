#include "bme280.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "threadx.h"
#define BME280_I2C_ADDR 0x76
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_STATUS 0xF3
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_HUM_MSB 0xFD

static int i2c_write(struct bme280_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t buf[256];
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) buf[1+i] = data[i];
    if (HAL_I2C_Master_Transmit(hi2c, (uint16_t)(dev->i2c_addr << 1), buf, 1+len, 100) != HAL_OK)
        return -1;
    return 0;
}

static int i2c_write_then_read(struct bme280_dev *dev, uint8_t reg, uint8_t *rx, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    if (HAL_I2C_Mem_Read(hi2c, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, rx, len, 100) != HAL_OK)
        return -1;
    return 0;
}

int bme280_init(struct bme280_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    uint8_t id;
    if (i2c_write_then_read(dev, BME280_REG_ID, &id, 1) != 0) return -1;
    if (id != 0x60) return -1;

    uint8_t reset_cmd = 0xB6;
    if (i2c_write(dev, BME280_REG_RESET, &reset_cmd, 1) != 0) return -1;
    HAL_Delay(2);

    uint8_t config_val = 0x00;
    if (i2c_write(dev, BME280_REG_CONFIG, &config_val, 1) != 0) return -1;

    uint8_t ctrl_hum_val = 0x01;
    if (i2c_write(dev, BME280_REG_CTRL_HUM, &ctrl_hum_val, 1) != 0) return -1;

    uint8_t ctrl_meas_val = 0x27;
    if (i2c_write(dev, BME280_REG_CTRL_MEAS, &ctrl_meas_val, 1) != 0) return -1;

    HAL_Delay(10);
    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t buf[8];
    if (i2c_write_then_read(dev, BME280_REG_PRESS_MSB, buf, 8) != 0) return -1;

    uint32_t press = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    press >>= 4;
    uint32_t temp = ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    temp >>= 4;
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;
    return 0;
}
