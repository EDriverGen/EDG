#include "mcp3008.h"
#include <stdint.h>

#include "riot.h"
void mcp3008_init(mcp3008_t *dev, spi_t bus) {
    dev->bus = bus;
    dev->cs = SPI_HWCS(0);
    spi_init_cs(bus, dev->cs);
}

uint16_t mcp3008_read_channel(mcp3008_t *dev, uint8_t channel) {
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | ((channel & 0x07) << 4);
    tx_buf[2] = 0x00;
    spi_transfer_bytes(dev->bus, dev->cs, false, tx_buf, rx_buf, 3);
    uint16_t raw = ((rx_buf[1] & 0x03) << 8) | rx_buf[2];
    return raw;
}
