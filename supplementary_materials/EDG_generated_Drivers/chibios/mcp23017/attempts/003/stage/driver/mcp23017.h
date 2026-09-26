#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>
#include "hal.h"

#define MCP23017_I2C_ADDR 0x20

#include "hal_i2c.h"
struct mcp23017_device {
    I2CDriver *bus_handle;
    uint8_t i2c_addr;
};

void mcp23017_init(struct mcp23017_device *dev, I2CDriver *bus_handle);
void mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta_byte);
void mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb_byte);

#endif
