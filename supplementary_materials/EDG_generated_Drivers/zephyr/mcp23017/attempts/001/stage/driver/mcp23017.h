#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
struct device;

int mcp23017_init(const struct device *dev);
int mcp23017_read_porta(const struct device *dev, uint8_t *porta_byte);
int mcp23017_read_portb(const struct device *dev, uint8_t *portb_byte);

#endif /* MCP23017_H */
