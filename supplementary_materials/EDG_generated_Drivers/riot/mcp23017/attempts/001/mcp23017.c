#include "mcp23017.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "periph/i2c.h"
#include "xtimer.h"

#include "riot.h"
#define MCP23017_I2C_ADDR 0x20
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13

int mcp23017_init(mcp23017_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = MCP23017_I2C_ADDR;

    xtimer_msleep(10);

    uint8_t iodira_data[2] = {MCP23017_IODIRA, 0x00};
    int ret = i2c_write_bytes(dev->bus, dev->addr, iodira_data, 2, 0);
    if (ret < 0) {
        return -EIO;
    }

    uint8_t iodirb_data[2] = {MCP23017_IODIRB, 0x00};
    ret = i2c_write_bytes(dev->bus, dev->addr, iodirb_data, 2, 0);
    if (ret < 0) {
        return -EIO;
    }

    return 0;
}

int mcp23017_read_porta(mcp23017_t *dev, uint8_t *porta) {
    uint8_t reg = MCP23017_GPIOA;
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, porta, 1, 0);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int mcp23017_read_portb(mcp23017_t *dev, uint8_t *portb) {
    uint8_t reg = MCP23017_GPIOB;
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, portb, 1, 0);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}
