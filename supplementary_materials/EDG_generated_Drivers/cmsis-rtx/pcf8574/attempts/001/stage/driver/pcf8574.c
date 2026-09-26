#include "pcf8574.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#define PCF8574_I2C_ADDR 0x20
#define I2C_TIMEOUT 100

int pcf8574_init(struct pcf8574_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL)
        return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_dev *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7)
{
    if (dev == NULL || dev->bus_handle == NULL)
        return -1;
    uint8_t data = 0;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), &data, 1, I2C_TIMEOUT);
    if (ret != HAL_OK)
        return -1;
    if (p0) *p0 = (data >> 0) & 1;
    if (p1) *p1 = (data >> 1) & 1;
    if (p2) *p2 = (data >> 2) & 1;
    if (p3) *p3 = (data >> 3) & 1;
    if (p4) *p4 = (data >> 4) & 1;
    if (p5) *p5 = (data >> 5) & 1;
    if (p6) *p6 = (data >> 6) & 1;
    if (p7) *p7 = (data >> 7) & 1;
    return 0;
}
