#include "mcp3008.h"
#include "transform.h"
#include <stddef.h>

int mcp3008_init(struct mcp3008_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out) {
    if (!dev || !out || channel > 7) return -1;
    uint8_t tx_buf[3];
    uint8_t rx_buf[3] = {0};
    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = 3,
        .speed_hz = 1350000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0
    };
    int ret = PrivTaskDelay(0);
    if (ret != 0) return -1;
    ret = PrivTaskDelay(0);
    if (ret != 0) return -1;
    *out = ((uint16_t)(rx_buf[1] & 0x03) << 8) | rx_buf[2];
    return 0;
}