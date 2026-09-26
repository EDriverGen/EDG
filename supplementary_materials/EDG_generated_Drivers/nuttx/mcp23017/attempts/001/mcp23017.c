#include "mcp23017.h"
#include <nuttx/i2c/i2c_master.h>
#include <errno.h>
#include <stdint.h>

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

static int mcp23017_write_reg(struct mcp23017_dev_s *dev, uint8_t reg, uint8_t value)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    uint8_t buf[2] = { reg, value };
    int ret = I2C_TRANSFER(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return -EIO;
    }
    return OK;
}

static int mcp23017_write_pointer(struct mcp23017_dev_s *dev, uint8_t reg)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_TRANSFER(dev->bus, &config, &reg, 1);
    if (ret < 0) {
        return -EIO;
    }
    return OK;
}

static int mcp23017_read_byte(struct mcp23017_dev_s *dev, uint8_t *value)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_TRANSFER(dev->bus, &config, value, 1);
    if (ret < 0) {
        return -EIO;
    }
    return OK;
}

int mcp23017_init(struct mcp23017_dev_s *dev, struct i2c_master_s *bus)
{
    if (!dev || !bus) {
        return -EINVAL;
    }
    dev->bus = bus;
    dev->addr = MCP23017_I2C_ADDR;
    int ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (ret < 0) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    if (ret < 0) return ret;
    return OK;
}

int mcp23017_read_gpioa(struct mcp23017_dev_s *dev, uint8_t *porta)
{
    if (!dev || !porta) {
        return -EINVAL;
    }
    int ret;
    ret = mcp23017_write_pointer(dev, MCP23017_GPIOA);
    if (ret < 0) return ret;
    ret = mcp23017_read_byte(dev, porta);
    if (ret < 0) return ret;
    return OK;
}

int mcp23017_read_gpiob(struct mcp23017_dev_s *dev, uint8_t *portb)
{
    if (!dev || !portb) {
        return -EINVAL;
    }
    int ret;
    ret = mcp23017_write_pointer(dev, MCP23017_GPIOB);
    if (ret < 0) return ret;
    ret = mcp23017_read_byte(dev, portb);
    if (ret < 0) return ret;
    return OK;
}