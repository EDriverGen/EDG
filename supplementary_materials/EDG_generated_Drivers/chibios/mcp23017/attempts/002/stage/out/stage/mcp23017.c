#include "mcp23017.h"
#include "hal_i2c.h"
#include <string.h>

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

static msg_t i2c_write(struct mcp23017_device *dev, const uint8_t *txbuf, size_t txbytes)
{
    i2cAcquireBus(dev->bus);
    msg_t ret = i2cMasterTransmitTimeout(dev->bus, dev->i2c_addr, txbuf, txbytes, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return ret;
}

static msg_t i2c_write_then_read(struct mcp23017_device *dev, const uint8_t *txbuf, size_t txbytes, uint8_t *rxbuf, size_t rxbytes)
{
    i2cAcquireBus(dev->bus);
    msg_t ret = i2cMasterTransmitTimeout(dev->bus, dev->i2c_addr, txbuf, txbytes, rxbuf, rxbytes, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return ret;
}

void mcp23017_init(struct mcp23017_device *dev, I2CDriver *bus)
{
    dev->bus = bus;
    dev->i2c_addr = MCP23017_I2C_ADDR;

    uint8_t txbuf[2];

    txbuf[0] = MCP23017_IODIRA;
    txbuf[1] = 0x00;
    i2c_write(dev, txbuf, 2);

    txbuf[0] = MCP23017_IODIRB;
    txbuf[1] = 0x00;
    i2c_write(dev, txbuf, 2);
}

void mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta_byte)
{
    uint8_t txbuf[1];
    txbuf[0] = MCP23017_GPIOA;
    i2c_write_then_read(dev, txbuf, 1, porta_byte, 1);
}

void mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb_byte)
{
    uint8_t txbuf[1];
    txbuf[0] = MCP23017_GPIOB;
    i2c_write_then_read(dev, txbuf, 1, portb_byte, 1);
}