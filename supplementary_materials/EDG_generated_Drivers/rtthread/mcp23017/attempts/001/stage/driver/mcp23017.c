#include "mcp23017.h"
#include <drivers/dev_i2c.h>
#include <stdint.h>
#include <errno.h>

#include "rtthread.h"
#define MCP23017_I2C_ADDR 0x20
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13

static int mcp23017_write_reg(struct mcp23017_device *dev, uint8_t reg, uint8_t data)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[2] = {reg, data};
    msgs[0].addr = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 2;
    msgs[0].buf = buf;
    if (rt_i2c_transfer(dev->bus, msgs, 1) != 1)
        return -EIO;
    return 0;
}

static int mcp23017_read_reg(struct mcp23017_device *dev, uint8_t reg, uint8_t *data)
{
    struct rt_i2c_msg msgs[2];
    uint8_t reg_buf = reg;
    msgs[0].addr = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 1;
    msgs[0].buf = &reg_buf;
    msgs[1].addr = dev->addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = 1;
    msgs[1].buf = data;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -EIO;
    return 0;
}

int mcp23017_init(struct mcp23017_device *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->addr = MCP23017_I2C_ADDR;
    int ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (ret) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    if (ret) return ret;
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
