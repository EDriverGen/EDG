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
    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    int ret = spi_bus_transfer(dev->bus, tx_buf, rx_buf, 3);
    if (ret != 0) {
        *out = 0;
        return ret;
    }

    uint8_t high_byte = rx_buf[1];
    uint8_t low_byte = rx_buf[2];
    uint16_t raw = ((uint16_t)(high_byte & 0x03) << 8) | low_byte;
    *out = raw;
    return 0;
}