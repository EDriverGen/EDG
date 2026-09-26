#include "pcf8574.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>
#include <stdint.h>

#include "threadx.h"
#define PCF8574_I2C_ADDR 0x20
#define I2C_TIMEOUT 100

int pcf8574_init(struct pcf8574_device *dev, void *bus_handle) {
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte) {
    if (dev == NULL || dev->bus_handle == NULL || port_byte == NULL) return -1;
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(hi2c, dev_addr, port_byte, 1, I2C_TIMEOUT);
    if (ret != HAL_OK) return -1;
    return 0;
}
