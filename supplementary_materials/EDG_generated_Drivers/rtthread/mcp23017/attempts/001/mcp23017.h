#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>

#include <drivers/dev_i2c.h>
struct rt_i2c_bus_device;

struct mcp23017_device {
    struct rt_i2c_bus_device *bus;
    uint8_t addr;
};

int mcp23017_init(struct mcp23017_device *dev, struct rt_i2c_bus_device *bus);
int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta);
int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb);

#endif /* MCP23017_H */
