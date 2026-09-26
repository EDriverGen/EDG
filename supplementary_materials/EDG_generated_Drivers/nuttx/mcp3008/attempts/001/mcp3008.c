#include "mcp3008.h"
#include <stdint.h>
#include <errno.h>

#include <nuttx/spi/spi.h>
#include "nuttx.h"
int mcp3008_init(struct mcp3008_dev_s *dev, struct spi_dev_s *spi)
{
    if (!dev || !spi)
        return -EINVAL;
    dev->spi = spi;
    return 0;
}

int mcp3008_read_channel(struct mcp3008_dev_s *dev, uint8_t channel, uint16_t *raw)
{
    if (!dev || !raw || channel > 7)
        return -EINVAL;

    struct spi_dev_s *spi = dev->spi;
    if (!spi)
        return -ENODEV;

    uint8_t tx_buf[3];
    uint8_t rx_buf[3];

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    SPI_LOCK(spi, true);
    SPI_SELECT(spi, 0, true);
    SPI_SETMODE(spi, SPIDEV_MODE0);
    SPI_SETBITS(spi, 8);
    SPI_SETFREQUENCY(spi, 1000000);

    int ret = SPI_EXCHANGE(spi, tx_buf, rx_buf, 3);

    SPI_SELECT(spi, 0, false);
    SPI_LOCK(spi, false);

    if (ret < 0)
        return ret;

    uint8_t high_byte = rx_buf[1];
    uint8_t low_byte = rx_buf[2];
    *raw = ((uint16_t)(high_byte & 0x03) << 8) | low_byte;

    return 0;
}
