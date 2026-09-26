#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

struct mcp3008_dev {
    int spi_num;
    int cs_pin;
};

int mcp3008_init(struct mcp3008_dev *dev, void *bus_handle);
int mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out);

#endif