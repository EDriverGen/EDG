#include "mcp3008.h"
#include "transform.h"
#include <stdint.h>
#include <string.h>

#define SPI_DEVICE_PATH "/dev/spi0"

int mcp3008_init(struct mcp3008_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    return 0;
}

int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *value) {
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    int ret;

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    ret = SpiTransferBytes(dev->bus_handle, tx_buf, rx_buf, 3);
    if (ret != 0) {
        return ret;
    }

    *value = ((uint16_t)(rx_buf[1] & 0x03) << 8) | rx_buf[2];
    return 0;
}