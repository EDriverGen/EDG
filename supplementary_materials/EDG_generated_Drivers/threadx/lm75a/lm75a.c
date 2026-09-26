#include "lm75a.h"
#include <stdint.h>
#include "stm32f1xx_hal_i2c.h"

#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG 0x00
#define LM75A_CONF_REG 0x01
#define LM75A_TOS_REG  0x03
#define LM75A_THYST_REG 0x02

static int lm75a_write_register(lm75a_device_t *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    uint8_t buf[3];
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    if (HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 1 + len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

static int lm75a_read_register(lm75a_device_t *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

int lm75a_init(lm75a_device_t *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    HAL_Delay(100);
    return 0;
}

int lm75a_read_temperature(lm75a_device_t *dev, int16_t *raw)
{
    uint8_t buf[2];
    if (lm75a_read_register(dev, LM75A_TEMP_REG, buf, 2) != 0) {
        return -1;
    }
    int16_t raw_val = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    raw_val >>= 5;
    if (raw_val & 0x0400) {
        raw_val |= 0xF800;
    }
    *raw = raw_val;
    return 0;
}
