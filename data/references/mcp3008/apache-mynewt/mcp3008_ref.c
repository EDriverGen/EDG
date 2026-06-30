#include "mcp3008_ref.h"

int mcp3008_init(struct mcp3008_device *dev, int spi_num, uint16_t vref_mv)
{
    struct hal_spi_settings settings;
    if (dev == 0) {
        return -1;
    }
    settings.data_mode = HAL_SPI_MODE0;
    settings.data_order = HAL_SPI_MSB_FIRST;
    settings.word_size = HAL_SPI_WORD_SIZE_8BIT;
    settings.baudrate = MCP3008_SPI_MAX_HZ;
    if (hal_spi_init(spi_num, 0, HAL_SPI_TYPE_MASTER) != 0 ||
        hal_spi_config(spi_num, &settings) != 0 ||
        hal_spi_enable(spi_num) != 0) {
        return -1;
    }
    dev->spi_num = spi_num;
    dev->vref_mv = vref_mv;
    return 0;
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3];
    uint8_t rx[3];

    if (dev == 0 || raw == 0) {
        return -1;
    }
    if (channel >= MCP3008_CHANNELS) {
        return -1;
    }

    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80U : 0x00U) | ((channel & 0x07U) << 4));
    tx[2] = 0x00;
    if (hal_spi_txrx(dev->spi_num, tx, rx, 3) != 0) {
        return -1;
    }
    *raw = (uint16_t)(((uint16_t)(rx[1] & 0x03U) << 8) | rx[2]);
    return 0;
}

int mcp3008_read_voltage(struct mcp3008_device *dev, uint8_t channel, uint16_t *mv)
{
    uint16_t raw = 0;
    if (dev == 0 || mv == 0) {
        return -1;
    }
    if (mcp3008_read_raw(dev, channel, MCP3008_SINGLE, &raw) != 0) {
        return -1;
    }
    return mcp3008_to_millivolts(raw, dev->vref_mv, mv);
}

int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv)
{
    if (mv == 0) {
        return -1;
    }
    *mv = (uint16_t)(((uint32_t)raw * vref_mv) / MCP3008_MAX_VALUE);
    return 0;
}
