#include "w25q64jv.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
static int spi_write_then_read(const struct spi_dt_spec *spec, const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len)
{
    struct spi_buf tx_buf = { .buf = (void *)tx_data, .len = tx_len };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
    struct spi_buf rx_buf = { .buf = rx_data, .len = rx_len };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    return spi_transceive_dt(spec, &tx, &rx);
}

int w25q64jv_init(const struct device *dev, const struct spi_dt_spec *spi_cfg)
{
    uint8_t cmd = 0x9F;
    uint8_t jedec[3];
    int ret;

    ret = spi_write_then_read(spi_cfg, &cmd, 1, jedec, 3);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int w25q64jv_read(const struct device *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    const struct spi_dt_spec *spec = (const struct spi_dt_spec *)dev;
    uint8_t tx_buf[4];
    tx_buf[0] = 0x03;
    tx_buf[1] = (addr >> 16) & 0xFF;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;

    struct spi_buf tx_bufs = { .buf = tx_buf, .len = 4 };
    struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };
    struct spi_buf rx_bufs = { .buf = buf, .len = len };
    struct spi_buf_set rx = { .buffers = &rx_bufs, .count = 1 };

    return spi_transceive_dt(spec, &tx, &rx);
}

int w25q64jv_write_page(const struct device *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    const struct spi_dt_spec *spec = (const struct spi_dt_spec *)dev;
    uint8_t cmd_enable = 0x06;
    int ret;

    ret = spi_write_then_read(spec, &cmd_enable, 1, NULL, 0);
    if (ret < 0) {
        return ret;
    }

    uint8_t tx_buf[4 + 256];
    tx_buf[0] = 0x02;
    tx_buf[1] = (addr >> 16) & 0xFF;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;
    for (size_t i = 0; i < len; i++) {
        tx_buf[4 + i] = buf[i];
    }

    struct spi_buf tx_bufs = { .buf = tx_buf, .len = 4 + len };
    struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };

    return spi_transceive_dt(spec, &tx, NULL);
}
