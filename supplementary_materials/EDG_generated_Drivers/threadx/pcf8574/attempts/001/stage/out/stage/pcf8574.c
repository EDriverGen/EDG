#include "pcf8574.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "threadx.h"
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_device *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte)
{
    if (dev == NULL || dev->bus_handle == NULL || port_byte == NULL) return -1;
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Receive(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), port_byte, 1, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}
