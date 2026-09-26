#include "mcp3008.h"
#include "hdf_base.h"
#include <stdint.h>

#include "spi_if.h"
int mcp3008_init(struct mcp3008_device *dev, DevHandle bus_handle)
{
    if (bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->spi_handle = bus_handle;
    return HDF_SUCCESS;
}

int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *out)
{
    if (dev == NULL || dev->spi_handle == NULL || out == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (channel > 7) {
        return HDF_ERR_INVALID_PARAM;
    }

    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    struct SpiMsg msg;
    msg.wbuf = tx_buf;
    msg.rbuf = rx_buf;
    msg.len = 3;
    msg.speed = 1350000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 1;

    int32_t ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    uint8_t high_byte = rx_buf[1];
    uint8_t low_byte = rx_buf[2];
    *out = (uint16_t)(((high_byte & 0x03) << 8) | low_byte);
    return HDF_SUCCESS;
}
