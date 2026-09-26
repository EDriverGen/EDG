/*
 * MCP3008 ADC driver for RT-Thread (SPI)
 */
#include "mcp3008_ref.h"

rt_err_t mcp3008_init(struct mcp3008_device *dev, const char *device_name, rt_uint16_t vref_mv)
{
    struct rt_spi_configuration cfg;
    if (dev == RT_NULL || device_name == RT_NULL) return -RT_EINVAL;
    dev->device_name = device_name;
    dev->vref_mv = vref_mv;
    dev->spi = (struct rt_spi_device *)rt_device_find(device_name);
    if (dev->spi == RT_NULL) return -RT_ENOSYS;
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.data_width = 8;
    cfg.max_hz = MCP3008_SPI_MAX_HZ;
    rt_spi_configure(dev->spi, &cfg);
    return RT_EOK;
}

rt_err_t mcp3008_read_raw(struct mcp3008_device *dev, rt_uint8_t channel, rt_uint8_t single, rt_uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    if (dev == RT_NULL || dev->spi == RT_NULL || raw == RT_NULL) return -RT_EINVAL;
    if (channel >= MCP3008_CHANNELS) return -RT_EINVAL;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    /* MCP3008 is a full-duplex 3-byte transaction: the ADC bytes are
     * clocked back on MISO during the same 3 byte slots that carry the
     * command on MOSI. send_then_recv would clock 6 bytes total and the
     * chip would not output the ADC bits where the driver expects them. */
    if (rt_spi_transfer(dev->spi, tx, rx, 3) != 3)
        return -RT_EIO;
    *raw = (rt_uint16_t)(((rx[1] & 0x03) << 8) | rx[2]);
    return RT_EOK;
}

rt_err_t mcp3008_read_voltage(struct mcp3008_device *dev, rt_uint8_t channel, rt_uint16_t *mv)
{
    rt_uint16_t raw;
    rt_err_t err;
    if (dev == RT_NULL || mv == RT_NULL) return -RT_EINVAL;
    err = mcp3008_read_raw(dev, channel, MCP3008_SINGLE, &raw);
    if (err != RT_EOK) return err;
    *mv = (rt_uint16_t)((rt_uint32_t)raw * dev->vref_mv / MCP3008_MAX_VALUE);
    return RT_EOK;
}

int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv)
{
    if (!mv) return -1;
    *mv = (uint16_t)((uint32_t)raw * vref_mv / MCP3008_MAX_VALUE);
    return 0;
}
