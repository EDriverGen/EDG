#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>
#include "periph/i2c.h"

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} mcp23017_t;

int mcp23017_init(mcp23017_t *dev, i2c_t bus);
int mcp23017_read_porta(mcp23017_t *dev, uint8_t *porta);
int mcp23017_read_portb(mcp23017_t *dev, uint8_t *portb);

#endif
