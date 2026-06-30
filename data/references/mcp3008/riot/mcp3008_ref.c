/*
 * MCP3008 ADC driver for RIOT (SPI)
 */
#include "mcp3008_ref.h"

int mcp3008_init(struct mcp3008_device *dev, spi_t bus, spi_cs_t cs, uint16_t vref_mv)
{
    if (!dev) return -1;
    dev->bus = bus; dev->cs = cs; dev->vref_mv = vref_mv;
    spi_init(bus); spi_init_cs(bus, cs);
    return 0;
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    if (!dev || !raw || channel >= MCP3008_CHANNELS) return -1;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_1MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, tx, rx, 3);
    spi_release(dev->bus);
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
