#include "mcp3008.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "tobudos.h"
void mcp3008_init(struct mcp3008_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
}

int mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out) {
    if (dev == NULL || dev->bus_handle == NULL || out == NULL || channel > 7) {
        return -1;
    }

    uint8_t tx_buf[3];
    uint8_t rx_buf[3];

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;

    if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 3, 100) != HAL_OK) {
        return -1;
    }

    uint8_t high_byte = rx_buf[1];
    uint8_t low_byte = rx_buf[2];
    *out = (uint16_t)(((high_byte & 0x03) << 8) | low_byte);

    return 0;
}
