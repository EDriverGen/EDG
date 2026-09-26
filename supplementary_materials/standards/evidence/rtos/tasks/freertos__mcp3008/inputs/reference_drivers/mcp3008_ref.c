/*
 * MCP3008 ADC driver for FreeRTOS + STM32 HAL
 */
#include "mcp3008_ref.h"

int mcp3008_init(struct mcp3008_device *dev, SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin, uint16_t vref_mv)
{
    if (!dev || !hspi || !cs_port) return -1;
    dev->hspi = hspi; dev->cs_port = cs_port; dev->cs_pin = cs_pin;
    dev->vref_mv = vref_mv;
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    return 0;
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    uint8_t tx[3], rx[3];
    if (!dev || !dev->hspi || !raw || channel >= MCP3008_CHANNELS) return -1;
    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80 : 0x00) | ((channel & 0x07) << 4));
    tx[2] = 0x00;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 3, 100);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
    if (st != HAL_OK) return -1;
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
