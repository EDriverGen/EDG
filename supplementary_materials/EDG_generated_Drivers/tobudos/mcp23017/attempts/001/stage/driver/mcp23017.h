#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>

struct mcp23017_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle);
int mcp23017_read_gpioa(struct mcp23017_dev *dev, uint8_t *porta_byte);
int mcp23017_read_gpiob(struct mcp23017_dev *dev, uint8_t *portb_byte);

#endif