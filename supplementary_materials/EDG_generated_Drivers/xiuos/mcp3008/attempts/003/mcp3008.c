#include "mcp3008.h"
#include "transform.h"
#include <stddef.h>

int mcp3008_init(struct mcp3008_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *value) {
    if (!dev || !dev->bus_handle || !value || channel > 7) return -1;

    uint8_t tx_buf[3];
    uint8_t rx_buf[3] = {0};

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    int ret = SpiTransferBytes(dev->bus_handle, tx_buf, rx_buf, 3);
    if (ret != 0) return -1;

    uint8_t high_byte = rx_buf[1];
    uint8_t low_byte = rx_buf[2];
    *value = (uint16_t)(((high_byte & 0x03) << 8) | low_byte);

    return 0;
}