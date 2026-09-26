#include "mcp23017.h"
#include "xtimer.h"
#include <errno.h>

#include "riot.h"
#define MCP23017_ADDR 0x20
#define IODIRA 0x00
#define IODIRB 0x01
#define GPIOA 0x12
#define GPIOB 0x13

int mcp23017_init(mcp23017_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = MCP23017_ADDR;
    uint8_t data[2];
    int ret;

    data[0] = IODIRA;
    data[1] = 0x00;
    ret = i2c_write_bytes(dev->bus, dev->addr, data, 2, 0);
    if (ret != 0) return -EIO;

    data[0] = IODIRB;
    data[1] = 0x00;
    ret = i2c_write_bytes(dev->bus, dev->addr, data, 2, 0);
    if (ret != 0) return -EIO;

    return 0;
}

int mcp23017_read_porta(mcp23017_t *dev, uint8_t *porta) {
    uint8_t reg = GPIOA;
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, porta, 1, 0);
    if (ret != 0) return -EIO;
    return 0;
}

int mcp23017_read_portb(mcp23017_t *dev, uint8_t *portb) {
    uint8_t reg = GPIOB;
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, portb, 1, 0);
    if (ret != 0) return -EIO;
    return 0;
}
