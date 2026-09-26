/*
 * MCP3008 ADC driver for XiUOS
 */
#include "mcp3008_ref.h"

int mcp3008_init(struct mcp3008_device *dev, const char *spi_path, uint16_t vref_mv)
{
    if (!dev || !spi_path) return -1;
    dev->spi_fd = PrivOpen(spi_path, O_RDWR);
    if (dev->spi_fd < 0) return -1;
    dev->vref_mv = vref_mv;
    return 0;
}

void mcp3008_deinit(struct mcp3008_device *dev)
{
    if (dev && dev->spi_fd >= 0) { PrivClose(dev->spi_fd); dev->spi_fd = -1; }
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    struct SpiDataParam xfer;
    if (!dev || dev->spi_fd < 0 || !raw || channel >= MCP3008_CHANNELS) return -1;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    xfer.tx_buff = tx;
    xfer.rx_buff = rx;
    xfer.length = 3;
    if (PrivIoctl(dev->spi_fd, SPI_IOC_TRANSFER, &xfer) != 0) return -1;
    *raw = (uint16_t)(((rx[1] & 0x03) << 8) | rx[2]);
    return 0;
}

int mcp3008_read_voltage(struct mcp3008_device *dev, uint8_t channel, uint16_t *mv)
{
    uint16_t raw;
    if (!dev || !mv) return -1;
    if (mcp3008_read_raw(dev, channel, MCP3008_SINGLE, &raw) != 0) return -1;
    *mv = (uint16_t)((uint32_t)raw * dev->vref_mv / MCP3008_MAX_VALUE);
    return 0;
}

int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv)
{
    if (!mv) return -1;
    *mv = (uint16_t)((uint32_t)raw * vref_mv / MCP3008_MAX_VALUE);
    return 0;
}
