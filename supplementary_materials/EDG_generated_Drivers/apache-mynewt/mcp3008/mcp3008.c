#include "mcp3008.h"
#include <assert.h>
#include <stdint.h>

#define MCP3008_SPI_MODE 0
#define MCP3008_SPI_BAUDRATE 1350000
#define MCP3008_SPI_WORD_SIZE 8

int mcp3008_init(struct mcp3008_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->spi_num = 0;
    dev->cs_pin = 0; /* board_configured_cs */
    return 0;
}

int mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out) {
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];
    int rc;

    tx_buf[0] = 0x01;
    tx_buf[1] = 0x80 | (channel << 4);
    tx_buf[2] = 0x00;

    struct hal_spi_settings spi_settings = {
        .data_mode = HAL_SPI_MODE0,
        .data_order = HAL_SPI_MSB_FIRST,
        .word_size = HAL_SPI_WORD_SIZE_8BIT,
        .baudrate = MCP3008_SPI_BAUDRATE
    };

    rc = hal_spi_config(dev->spi_num, &spi_settings);
    assert(rc == 0);

    rc = hal_spi_txrx(dev->spi_num, tx_buf, rx_buf, 3);
    assert(rc == 0);

    uint16_t high_byte = rx_buf[1];
    uint16_t low_byte = rx_buf[2];
    *out = ((high_byte & 0x03) * 256) + low_byte;

    return 0;
}