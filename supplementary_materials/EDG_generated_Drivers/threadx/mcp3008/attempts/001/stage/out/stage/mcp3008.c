#include "mcp3008.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "threadx.h"
int mcp3008_init(struct mcp3008_device *dev, void *bus_handle) {
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out) {
    if (dev == NULL || dev->bus_handle == NULL || out == NULL || channel > 7) return -1;
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 3, 100);
    if (ret != HAL_OK) return -1;
    uint16_t raw = ((uint16_t)(rx_buf[1] & 0x03) << 8) | (uint16_t)rx_buf[2];
    *out = raw;
    return 0;
}
