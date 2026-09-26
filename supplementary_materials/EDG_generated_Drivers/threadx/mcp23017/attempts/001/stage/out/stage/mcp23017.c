#include "mcp23017.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "threadx.h"
#define MCP23017_I2C_ADDR 0x20
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = MCP23017_I2C_ADDR;

    uint8_t iodira_data[2] = {MCP23017_IODIRA, 0x00};
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, iodira_data, 2, 100) != HAL_OK)
        return -1;

    uint8_t iodirb_data[2] = {MCP23017_IODIRB, 0x00};
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, iodirb_data, 2, 100) != HAL_OK)
        return -1;

    return 0;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_byte)
{
    if (dev == NULL || dev->bus_handle == NULL || porta_byte == NULL) return -1;

    uint8_t reg = MCP23017_GPIOA;
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, &reg, 1, 100) != HAL_OK)
        return -1;

    if (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, porta_byte, 1, 100) != HAL_OK)
        return -1;

    return 0;
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_byte)
{
    if (dev == NULL || dev->bus_handle == NULL || portb_byte == NULL) return -1;

    uint8_t reg = MCP23017_GPIOB;
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, &reg, 1, 100) != HAL_OK)
        return -1;

    if (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, portb_byte, 1, 100) != HAL_OK)
        return -1;

    return 0;
}
