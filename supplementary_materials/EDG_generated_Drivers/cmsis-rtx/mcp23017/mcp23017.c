#include "mcp23017.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#define MCP23017_I2C_ADDR 0x20
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13

static int i2c_write(struct mcp23017_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, data, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

static int i2c_write_then_read(struct mcp23017_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, buf, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = MCP23017_I2C_ADDR;
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = 0x00;
    if (i2c_write(dev, MCP23017_IODIRA, data, 2) != 0) return -1;
    data[0] = 0x01;
    data[1] = 0x00;
    if (i2c_write(dev, MCP23017_IODIRB, data, 2) != 0) return -1;
    return 0;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_byte)
{
    return i2c_write_then_read(dev, MCP23017_GPIOA, porta_byte, 1);
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_byte)
{
    return i2c_write_then_read(dev, MCP23017_GPIOB, portb_byte, 1);
}
