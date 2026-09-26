#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

#include "riot.h"
typedef struct {
    spi_t bus;
    spi_cs_t cs;
} mcp3008_t;

void mcp3008_init(mcp3008_t *dev, spi_t bus);
uint16_t mcp3008_read_channel(mcp3008_t *dev, uint8_t channel);

#endif /* MCP3008_H */
