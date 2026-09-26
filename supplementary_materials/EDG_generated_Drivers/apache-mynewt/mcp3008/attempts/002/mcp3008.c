#include "mcp3008.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <os/os_time.h>
int mcp3008_init(struct mcp3008_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->spi_num = 0;
    dev->cs_pin = board_configured_cs;
    hal_gpio_init_out(dev->cs_pin, 1);
    os_time_delay(1);
    return 0;
}

int mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out) {
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;
    hal_gpio_write(dev->cs_pin, 0);
    int ret = hal_spi_txrx(dev->spi_num, tx_buf, rx_buf, 3);
    hal_gpio_write(dev->cs_pin, 1);
    if (ret != 0) {
        return -1;
    }
    uint16_t raw = ((uint16_t)(rx_buf[1] & 0x03) << 8) | rx_buf[2];
    *out = raw;
    return 0;
}
