#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

#include "rtthread.h"
struct rt_spi_device;

struct mcp3008_device {
    struct rt_spi_device *spi;
};

int mcp3008_init(struct mcp3008_device *dev, struct rt_spi_device *spi);
int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *value);

#endif /* MCP3008_H */
