#include "mcp3008.h"
#include <stdint.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/sys/byteorder.h>
int mcp3008_init(struct mcp3008_data *dev, const struct spi_dt_spec *spi_spec)
{
	if (!dev || !spi_spec)
		return -1;
	dev->spi = spi_spec;
	return 0;
}

int mcp3008_read_channel(struct mcp3008_data *dev, uint8_t channel, uint16_t *result)
{
	if (!dev || !dev->spi || !result || channel > 7)
		return -1;

	uint8_t tx_buf[3] = {0x01, (uint8_t)(0x80 | (channel << 4)), 0x00};
	uint8_t rx_buf[3] = {0};

	const struct spi_buf tx_bufs = {.buf = tx_buf, .len = 3};
	const struct spi_buf rx_bufs = {.buf = rx_buf, .len = 3};
	const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_bufs, .count = 1};

	int ret = spi_transceive_dt(dev->spi, &tx, &rx);
	if (ret < 0)
		return ret;

	uint16_t raw = ((uint16_t)(rx_buf[1] & 0x03) << 8) | rx_buf[2];
	*result = raw;
	return 0;
}
