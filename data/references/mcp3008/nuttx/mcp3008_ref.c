/*
 * MCP3008 ADC driver for NuttX (SPI)
 */
#include "mcp3008_ref.h"
#include <errno.h>

int mcp3008_init(FAR struct mcp3008_device *dev, FAR struct spi_dev_s *spi,
                 uint32_t devid, uint16_t vref_mv)
{
    if (!dev || !spi) return -EINVAL;
    dev->spi = spi; dev->devid = devid; dev->vref_mv = vref_mv;
    return 0;
}

int mcp3008_read_raw(FAR struct mcp3008_device *dev, uint8_t channel, uint8_t single, FAR uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    if (!dev || !dev->spi || !raw || channel >= MCP3008_CHANNELS) return -EINVAL;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    SPI_LOCK(dev->spi, true);
    SPI_SETFREQUENCY(dev->spi, MCP3008_SPI_MAX_HZ);
    SPI_SETMODE(dev->spi, SPIDEV_MODE0);
    SPI_SETBITS(dev->spi, 8);
    SPI_SELECT(dev->spi, dev->devid, true);
    SPI_EXCHANGE(dev->spi, tx, rx, 3);
    SPI_SELECT(dev->spi, dev->devid, false);
    SPI_LOCK(dev->spi, false);
    *raw = (uint16_t)(((rx[1] & 0x03) << 8) | rx[2]);
    return 0;
}

int mcp3008_read_voltage(FAR struct mcp3008_device *dev, uint8_t channel, FAR uint16_t *mv)
{
    uint16_t raw; int ret;
    if (!dev || !mv) return -EINVAL;
    ret = mcp3008_read_raw(dev, channel, MCP3008_SINGLE, &raw);
    if (ret) return ret;
    *mv = (uint16_t)((uint32_t)raw * dev->vref_mv / MCP3008_MAX_VALUE);
    return 0;
}

int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv)
{
    if (!mv) return -1;
    *mv = (uint16_t)((uint32_t)raw * vref_mv / MCP3008_MAX_VALUE);
    return 0;
}
