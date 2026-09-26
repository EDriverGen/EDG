#include "mcp3008.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

void mcp3008_init(struct mcp3008_device *dev, spi_bus bus) {
    dev->bus = bus;
}

uint16_t mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out) {
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    int ret;

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    ret = spi_bus_transfer(dev->bus, tx_buf, rx_buf, 3);
    if (ret < 0) {
        return (uint16_t)(-ret);
    }

    *out = ((uint16_t)(rx_buf[1] & 0x03) << 8) | rx_buf[2];
    return 0;
}