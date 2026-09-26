#include "mcp3008.h"
#include <stdint.h>
#include <string.h>

void mcp3008_init(struct mcp3008_device *dev, spi_bus bus) {
    dev->bus = bus;
}

uint16_t mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out) {
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    memset(tx_buf, 0, sizeof(tx_buf));
    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    spi_bus_transfer(dev->bus, tx_buf, rx_buf, 3);

    uint16_t raw = ((rx_buf[1] & 0x03) << 8) | rx_buf[2];
    *out = raw;
    return 0;
}