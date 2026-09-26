#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>
#include "bus_i2c.h"

#define MCP23017_I2C_ADDR 0x20

struct I2cBus;

struct mcp23017_dev {
    struct I2cBus *bus;
    int fd;
};

int mcp23017_init(struct mcp23017_dev *dev, struct I2cBus *bus);
int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta);
int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb);

#endif