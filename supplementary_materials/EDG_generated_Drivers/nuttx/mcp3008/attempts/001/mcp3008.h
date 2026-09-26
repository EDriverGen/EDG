#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

struct spi_dev_s;

struct mcp3008_dev_s {
    struct spi_dev_s *spi;
};

int mcp3008_init(struct mcp3008_dev_s *dev, struct spi_dev_s *spi);
int mcp3008_read_channel(struct mcp3008_dev_s *dev, uint8_t channel, uint16_t *raw);

#endif /* MCP3008_H */