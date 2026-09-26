#include "mcp3008.h"
#include <stdint.h>

#include "freertos.h"
void mcp3008_init(struct mcp3008_dev *dev, void *bus_handle)
{
    dev->hspi = (SPI_HandleTypeDef *)bus_handle;
    dev->cs_port = GPIOA;
    dev->cs_pin = GPIO_PIN_4;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

void mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out)
{
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    uint8_t config = 0x80 | (channel << 4);
    tx_buf[0] = 0x01;
    tx_buf[1] = config;
    tx_buf[2] = 0x00;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(dev->hspi, tx_buf, rx_buf, 3, 100);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    uint8_t high_byte = rx_buf[1];
    uint8_t low_byte = rx_buf[2];
    *out = (uint16_t)(((high_byte & 0x03) << 8) | low_byte);
}
