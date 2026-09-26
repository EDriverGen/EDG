/*
 * MCP3008 ADC driver for ChibiOS (SPI)
 */
#include "mcp3008_ref.h"

int mcp3008_init(struct mcp3008_device *dev, SPIDriver *spid, const SPIConfig *cfg, uint16_t vref_mv)
{
    if (!dev || !spid || !cfg) return -1;
    dev->spid = spid; dev->spi_cfg = cfg; dev->vref_mv = vref_mv;
    return 0;
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    if (!dev || !dev->spid || !raw || channel >= MCP3008_CHANNELS) return -1;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    spiAcquireBus(dev->spid);
    spiStart(dev->spid, dev->spi_cfg);
    spiSelect(dev->spid);
    spiExchange(dev->spid, 3, tx, rx);
    spiUnselect(dev->spid);
    spiReleaseBus(dev->spid);
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
