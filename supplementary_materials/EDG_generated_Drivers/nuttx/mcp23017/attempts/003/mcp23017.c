#include "mcp23017.h"
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

static int mcp23017_write_reg(struct mcp23017_dev_s *dev, uint8_t reg, uint8_t val)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    uint8_t buf[2] = { reg, val };
    int ret = I2C_TRANSFER(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int mcp23017_read_reg(struct mcp23017_dev_s *dev, uint8_t reg, uint8_t *val)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_WRITE(dev->bus, &config, &reg, 1);
    if (ret < 0) {
        return -EIO;
    }
    ret = I2C_READ(dev->bus, &config, val, 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int mcp23017_init(struct mcp23017_dev_s *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = MCP23017_I2C_ADDR;
    int ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (ret < 0) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    if (ret < 0) return ret;
    return 0;
}

int mcp23017_read_gpioa(struct mcp23017_dev_s *dev, uint8_t *val)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOA, val);
}

int mcp23017_read_gpiob(struct mcp23017_dev_s *dev, uint8_t *val)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOB, val);
}