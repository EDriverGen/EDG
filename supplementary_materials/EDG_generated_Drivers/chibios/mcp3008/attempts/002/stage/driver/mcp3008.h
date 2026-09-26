#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

struct mcp3008_device {
    void *spi_handle;
};

void mcp3008_init(struct mcp3008_device *dev, void *spi_handle);
void mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out);

#endif /* MCP3008_H */