#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include "i2c_if.h"

#define MCP23017_I2C_ADDR 0x20

#include "openharmony_liteosm.h"
struct mcp23017_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int mcp23017_init(struct mcp23017_dev *dev, DevHandle bus_handle);
int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_byte);
int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_byte);

#endif
