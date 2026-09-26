#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>

#define MCP23017_I2C_ADDR 0x20

struct mcp23017_device {
    int fd;
    uint8_t i2c_addr;
};

int mcp23017_init(struct mcp23017_device *dev, void *bus_handle);
int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta_byte);
int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb_byte);

#endif /* MCP23017_H */