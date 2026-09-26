#include "pcf8574.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>
#include <stddef.h>

#define PCF8574_I2C_ADDR 0x20
#define PCF8574_TIMEOUT 100

int pcf8574_init(struct pcf8574_dev *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_dev *dev, uint8_t *port_byte)
{
    if (!dev || !dev->bus_handle || !port_byte) return -1;
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(hi2c, addr, port_byte, 1, PCF8574_TIMEOUT);
    if (ret != HAL_OK) return -1;
    return 0;
}