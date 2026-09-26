#include "mcp3008_ref.h"

int mcp3008_init(struct mcp3008_device *dev, SPI_HandleTypeDef *spi, uint16_t vref_mv)
{
    if (dev == 0 || spi == 0) {
        return -1;
    }
    if (HAL_SPI_Init(spi) != HAL_OK) {
        return -1;
    }
    dev->spi = spi;
    dev->vref_mv = vref_mv;
    return 0;
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3];
    uint8_t rx[3];

    if (dev == 0 || dev->spi == 0 || raw == 0) {
        return -1;
    }
    if (channel >= MCP3008_CHANNELS) {
        return -1;
    }

    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80U : 0x00U) | ((channel & 0x07U) << 4));
    tx[2] = 0x00;
    if (HAL_SPI_TransmitReceive(dev->spi, tx, rx, 3, 100) != HAL_OK) {
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
