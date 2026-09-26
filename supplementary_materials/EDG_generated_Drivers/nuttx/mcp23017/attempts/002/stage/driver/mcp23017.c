#include "mcp23017.h"
#include <nuttx/i2c/i2c_master.h>
#include <errno.h>
#include <stdint.h>

#define MCP23017_ADDR 0x20
#define IODIRA 0x00
#define IODIRB 0x01
#define GPIOA 0x12
#define GPIOB 0x13

static int mcp23017_write_reg(struct mcp23017_dev_s *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg_s msg;
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    msg.frequency = 100000;
    msg.addr = MCP23017_ADDR;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 2;
    return I2C_TRANSFER(dev->bus, &msg, 1);
}

static int mcp23017_read_reg(struct mcp23017_dev_s *dev, uint8_t reg, uint8_t *val)
{
    struct i2c_msg_s msg[2];
    uint8_t reg_buf = reg;
    msg[0].frequency = 100000;
    msg[0].addr = MCP23017_ADDR;
    msg[0].flags = 0;
    msg[0].buffer = &reg_buf;
    msg[0].length = 1;
    msg[1].frequency = 100000;
    msg[1].addr = MCP23017_ADDR;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = val;
    msg[1].length = 1;
    return I2C_TRANSFER(dev->bus, msg, 2);
}

int mcp23017_init(struct mcp23017_dev_s *dev, struct i2c_master_s *bus)
{
    int ret;
    dev->bus = bus;
    dev->addr = MCP23017_ADDR;
    ret = mcp23017_write_reg(dev, IODIRA, 0x00);
    if (ret < 0) return ret;
    ret = mcp23017_write_reg(dev, IODIRB, 0x00);
    return ret;
}

int mcp23017_read_gpioa(struct mcp23017_dev_s *dev, uint8_t *porta)
{
    return mcp23017_read_reg(dev, GPIOA, porta);
}

int mcp23017_read_gpiob(struct mcp23017_dev_s *dev, uint8_t *portb)
{
    return mcp23017_read_reg(dev, GPIOB, portb);
}