#include "mcp23017.h"
#include "rtthread.h"
#include <drivers/dev_i2c.h>
#include <errno.h>
#include <stdint.h>

#define MCP23017_ADDR 0x20
#define IODIRA 0x00
#define IODIRB 0x01
#define GPIOA 0x12
#define GPIOB 0x13

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
    uint8_t wbuf[1] = {reg};
    msgs[0].addr = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 1;
    msgs[0].buf = wbuf;
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
    int ret;
    dev->bus = bus;
    dev->addr = MCP23017_ADDR;
    ret = mcp23017_write_reg(dev, IODIRA, 0x00);
    if (ret) return ret;
    ret = mcp23017_write_reg(dev, IODIRB, 0x00);
    return ret;
}

int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta)
{
    return mcp23017_read_reg(dev, GPIOA, porta);
}

int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb)
{
    return mcp23017_read_reg(dev, GPIOB, portb);
}