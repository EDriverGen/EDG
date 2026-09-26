#include "mcp3008.h"
#include "ch.h"
#include "hal.h"
#include <stddef.h>

#include "chibios.h"
void mcp3008_init(struct mcp3008_device *dev, void *spi_handle) {
    dev->spi_handle = spi_handle;
}

void mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out) {
    uint8_t txbuf[3];
    uint8_t rxbuf[3];
    txbuf[0] = 0x01;
    txbuf[1] = 0x80 | (channel << 4);
    txbuf[2] = 0x00;
    spiExchange(dev->spi_handle, 3, txbuf, rxbuf);
    uint16_t raw = ((uint16_t)(rxbuf[1] & 0x03) << 8) | rxbuf[2];
    *out = raw;
}
