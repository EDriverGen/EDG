#include "mcp3008.h"
#include <stdint.h>

#include "cmsis_rtx.h"
void mcp3008_init(struct mcp3008_dev *dev, void *bus_handle) {
    dev->hspi = (SPI_HandleTypeDef *)bus_handle;
}

void mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out) {
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    uint16_t raw;

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    HAL_SPI_TransmitReceive(dev->hspi, tx_buf, rx_buf, 3, 100);

    raw = ((uint16_t)(rx_buf[1] & 0x03) << 8) | (uint16_t)rx_buf[2];
    *out = raw;
}
