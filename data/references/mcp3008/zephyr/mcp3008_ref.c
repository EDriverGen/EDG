/*
 * MCP3008 ADC driver for Zephyr (SPI)
 */
#include "mcp3008_ref.h"

int mcp3008_init(struct mcp3008_device *dev, const struct device *spi,
                 const struct gpio_dt_spec *cs_gpio, uint16_t vref_mv)
{
    if (!dev || !spi) return -EINVAL;
    if (!device_is_ready(spi)) return -ENODEV;
    dev->spi_dev = spi; dev->vref_mv = vref_mv;
    dev->spi_cfg.frequency = MCP3008_SPI_MAX_HZ;
    dev->spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER;
    dev->spi_cfg.slave = 0;
    if (cs_gpio) { dev->cs_ctrl.gpio = *cs_gpio; dev->cs_ctrl.delay = 0;
                   dev->spi_cfg.cs = &dev->cs_ctrl; }
    else dev->spi_cfg.cs = NULL;
    return 0;
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    struct spi_buf tx_buf = { .buf = tx, .len = 3 };
    struct spi_buf rx_buf = { .buf = rx, .len = 3 };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
    struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };
    if (!dev || !dev->spi_dev || !raw || channel >= MCP3008_CHANNELS) return -EINVAL;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    int ret = spi_transceive(dev->spi_dev, &dev->spi_cfg, &tx_set, &rx_set);
    if (ret < 0) return ret;
    *raw = (uint16_t)(((rx[1] & 0x03) << 8) | rx[2]);
    return 0;
}

int mcp3008_read_voltage(struct mcp3008_device *dev, uint8_t channel, uint16_t *mv)
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
