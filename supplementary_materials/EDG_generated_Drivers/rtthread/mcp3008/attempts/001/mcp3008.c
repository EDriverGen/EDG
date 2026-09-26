#include "mcp3008.h"
#include "rtthread.h"
#include "rtdevice.h"
#include <stdint.h>

int mcp3008_init(struct mcp3008_device *dev, struct rt_spi_device *spi)
{
    if (dev == RT_NULL || spi == RT_NULL) {
        return -1;
    }
    dev->spi = spi;
    return 0;
}

int mcp3008_read_channel(struct mcp3008_device *dev, uint8_t channel, uint16_t *value)
{
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    uint16_t raw;
    rt_ssize_t ret;

    if (dev == RT_NULL || dev->spi == RT_NULL || value == RT_NULL) {
        return -1;
    }

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    ret = rt_spi_transfer(dev->spi, tx_buf, rx_buf, 3);
    if (ret != 3) {
        return -1;
    }

    raw = ((uint16_t)(rx_buf[1] & 0x03) << 8) | rx_buf[2];
    *value = raw;
    return 0;
}