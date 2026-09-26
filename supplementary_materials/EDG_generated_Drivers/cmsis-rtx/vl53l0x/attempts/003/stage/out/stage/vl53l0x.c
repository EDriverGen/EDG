#include "vl53l0x.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "cmsis_rtx.h"
#define VL53L0X_I2C_ADDR 0x52

static int32_t vl53l0x_read_reg(vl53l0x_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    if (HAL_I2C_Master_Transmit(hi2c, (uint16_t)(dev->i2c_addr << 1), &reg, 1, 100) != HAL_OK)
        return -1;
    if (HAL_I2C_Master_Receive(hi2c, (uint16_t)(dev->i2c_addr << 1), buf, len, 100) != HAL_OK)
        return -1;
    return 0;
}

int32_t vl53l0x_init(vl53l0x_t *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    HAL_Delay(2);
    return 0;
}

int32_t vl53l0x_read_distance(vl53l0x_t *dev, int32_t *raw)
{
    uint8_t buf[2];
    uint8_t reg = 0x00;
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    if (HAL_I2C_Master_Transmit(hi2c, (uint16_t)(dev->i2c_addr << 1), &reg, 1, 100) != HAL_OK)
        return -1;
    if (HAL_I2C_Master_Receive(hi2c, (uint16_t)(dev->i2c_addr << 1), buf, 2, 100) != HAL_OK)
        return -1;
    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
