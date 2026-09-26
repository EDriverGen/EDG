#include "mcp23017.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "freertos.h"
#define MCP23017_I2C_ADDR 0x20
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13

static int mcp23017_write_reg(struct mcp23017_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     buf, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int mcp23017_read_reg(struct mcp23017_dev *dev, uint8_t reg, uint8_t *data)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle,
                                  (uint16_t)(dev->i2c_addr << 1),
                                  data, 1, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = MCP23017_I2C_ADDR;
    HAL_Delay(10);
    int ret = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (ret != 0) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    return ret;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_val)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOA, porta_val);
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_val)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOB, portb_val);
}
