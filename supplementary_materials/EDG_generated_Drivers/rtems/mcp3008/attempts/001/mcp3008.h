#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

#include <dev/spi/spi.h>
struct mcp3008_device {
    spi_bus bus;
};

void mcp3008_init(struct mcp3008_device *dev, spi_bus bus);
uint16_t mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out);

#endif /* MCP3008_H */
