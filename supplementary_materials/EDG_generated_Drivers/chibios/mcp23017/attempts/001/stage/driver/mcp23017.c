#include "mcp23017.h"
#include "hal.h"
#include <string.h>

#include "hal_i2c.h"
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

static int mcp23017_write_reg(struct mcp23017_device *dev, uint8_t reg, uint8_t data)
{
    uint8_t txbuf[2] = { reg, data };
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle,
                                          dev->i2c_addr,
                                          txbuf, 2,
                                          NULL, 0,
                                          TIME_MS2I(100));
    return (ret == MSG_OK) ? 0 : -1;
}

static int mcp23017_read_reg(struct mcp23017_device *dev, uint8_t reg, uint8_t *data)
{
    uint8_t txbuf[1] = { reg };
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle,
                                          dev->i2c_addr,
                                          txbuf, 1,
                                          data, 1,
                                          TIME_MS2I(100));
    return (ret == MSG_OK) ? 0 : -1;
}

int mcp23017_init(struct mcp23017_device *dev, void *bus_handle)
{
    dev->bus_handle = (struct I2CDriver *)bus_handle;
    dev->i2c_addr = MCP23017_I2C_ADDR;

    /* Set all GPA pins as outputs */
    if (mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00) != 0)
        return -1;
    /* Set all GPB pins as outputs */
    if (mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00) != 0)
        return -1;

    return 0;
}

int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOA, porta);
}

int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOB, portb);
}
