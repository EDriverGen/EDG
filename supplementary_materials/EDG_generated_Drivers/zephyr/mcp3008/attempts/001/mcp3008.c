#include "mcp3008.h"
#include <zephyr/drivers/spi.h>
#include <stdint.h>

#include <zephyr/sys/byteorder.h>
int mcp3008_init(struct mcp3008_data *dev, const struct spi_dt_spec *spi_spec)
{
	if (!dev || !spi_spec)
		return -EINVAL;

	dev->spi_dev = spi_spec->bus;
	dev->spi_cfg = spi_spec->config;
	dev->spi_cfg.frequency = 1350000;
	dev->spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	dev->spi_cfg.slave = 0;

	return 0;
}

int mcp3008_read_channel(struct mcp3008_data *dev, uint8_t channel, uint16_t *result)
{
	if (!dev || !result || channel > 7)
		return -EINVAL;

	uint8_t tx_buf[3];
	uint8_t rx_buf[3];

	tx_buf[0] = 0x01;
	tx_buf[1] = 0x80 | (channel << 4);
	tx_buf[2] = 0x00;

	const struct spi_buf tx_bufs = {
		.buf = tx_buf,
		.len = 3
	};
	const struct spi_buf rx_bufs = {
		.buf = rx_buf,
		.len = 3
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_bufs,
		.count = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_bufs,
		.count = 1
	};

	int ret = spi_transceive_dt(&(struct spi_dt_spec){
				.bus = dev->spi_dev,
				.config = dev->spi_cfg
			}, &tx, &rx);
	if (ret < 0)
		return ret;

	uint8_t high_byte = rx_buf[1];
	uint8_t low_byte = rx_buf[2];
	*result = ((uint16_t)(high_byte & 0x03) << 8) | low_byte;

	return 0;
}
