#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>
#include <zephyr/drivers/spi.h>

struct device;
struct spi_dt_spec;

struct mcp3008_data {
	const struct device *spi_dev;
	struct spi_config spi_cfg;
};

int mcp3008_init(struct mcp3008_data *dev, const struct spi_dt_spec *spi_spec);
int mcp3008_read_channel(struct mcp3008_data *dev, uint8_t channel, uint16_t *result);

#endif /* MCP3008_H */