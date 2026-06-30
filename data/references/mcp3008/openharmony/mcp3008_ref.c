/*
 * MCP3008 ADC driver for OpenHarmony HDF (SPI)
 */
#include "mcp3008_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG mcp3008

int32_t mcp3008_init(struct mcp3008_device *dev, uint32_t bus, uint32_t cs, uint16_t vref_mv)
{
    struct SpiDevInfo info = { .busNum = bus, .csNum = cs };
    if (!dev) return HDF_ERR_INVALID_PARAM;
    dev->spi_handle = SpiOpen(&info);
    if (!dev->spi_handle) return HDF_FAILURE;
    dev->vref_mv = vref_mv;
    return HDF_SUCCESS;
}

void mcp3008_deinit(struct mcp3008_device *dev)
{
    if (dev && dev->spi_handle) { SpiClose(dev->spi_handle); dev->spi_handle = NULL; }
}

int32_t mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    struct SpiMsg msg = { .wbuf=tx, .rbuf=rx, .len=3, .speed=MCP3008_SPI_MAX_HZ, .keepCs=0 };
    if (!dev || !dev->spi_handle || !raw || channel >= MCP3008_CHANNELS) return HDF_ERR_INVALID_PARAM;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    if (SpiTransfer(dev->spi_handle, &msg, 1) != HDF_SUCCESS) return HDF_FAILURE;
    *raw = (uint16_t)(((rx[1] & 0x03) << 8) | rx[2]);
    return HDF_SUCCESS;
}

int32_t mcp3008_read_voltage(struct mcp3008_device *dev, uint8_t channel, uint16_t *mv)
{
    uint16_t raw; int32_t ret;
    if (!dev || !mv) return HDF_ERR_INVALID_PARAM;
    ret = mcp3008_read_raw(dev, channel, MCP3008_SINGLE, &raw);
    if (ret != HDF_SUCCESS) return ret;
    *mv = (uint16_t)((uint32_t)raw * dev->vref_mv / MCP3008_MAX_VALUE);
    return HDF_SUCCESS;
}

int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv)
{
    if (!mv) return -1;
    *mv = (uint16_t)((uint32_t)raw * vref_mv / MCP3008_MAX_VALUE);
    return 0;
}
