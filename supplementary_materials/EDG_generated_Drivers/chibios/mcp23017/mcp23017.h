#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>

#include "hal_i2c.h"
/* Forward declaration of I2CDriver from ChibiOS */
struct I2CDriver;

#define MCP23017_I2C_ADDR 0x20

struct mcp23017_device {
    struct I2CDriver *bus_handle;
    uint8_t i2c_addr;
};

int mcp23017_init(struct mcp23017_device *dev, void *bus_handle);
int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta);
int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb);

#endif /* MCP23017_H */
