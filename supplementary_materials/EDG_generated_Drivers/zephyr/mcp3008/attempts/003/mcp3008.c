#include "mcp3008.h"
#include <stdint.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/sys/byteorder.h>
int mcp3008_init(struct mcp3008_data *dev, const struct spi_dt_spec *spi_spec)
{
	dev->spi_spec = spi_spec;
	return 0;
}

int mcp3008_read_channel(struct mcp3008_data *dev, uint8_t channel, uint16_t *result)
{
	uint8_t tx_buf[3];
	uint8_t rx_buf[3];
	const struct spi_buf tx_bufs[] = {
		{
			.buf = tx_buf,
			.len = sizeof(tx_buf)
		}
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = rx_buf,
			.len = sizeof(rx_buf)
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = 1
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = 1
	};
	int ret;

	tx_buf[0] = 0x01;
	tx_buf[1] = 0x80 | (channel << 4);
	tx_buf[2] = 0x00;

	ret = spi_transceive_dt(dev->spi_spec, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	*result = ((uint16_t)(rx_buf[1] & 0x03) << 8) | (uint16_t)rx_buf[2];
	return 0;
}
