#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

#include "spi_if.h"
struct mcp3008_device {
    DevHandle spi_handle;
};

int mcp3008_init(struct mcp3008_device *dev, DevHandle bus_handle);
int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out);

#endif
