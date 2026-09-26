#include "w25q64jv.h"
#include <errno.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/spi.h>
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
    /* JEDEC ID expected: 0xEF 0x40 0x17 for W25Q64JV */
    if (jedec[0] != 0xEF || jedec[1] != 0x40 || jedec[2] != 0x17) {
        return -ENODEV;
    }
    return 0;
}

int w25q64jv_read(const struct device *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    const struct spi_dt_spec *spec = (const struct spi_dt_spec *)dev;
    uint8_t tx_buf[4];
    uint8_t rx_buf[4];
    int ret;

    if (len == 0) {
        return 0;
    }

    tx_buf[0] = 0x03;
    tx_buf[1] = (addr >> 16) & 0xFF;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;

    /* Send command + address, then read data in same transaction */
    struct spi_buf tx_bufs[2];
    struct spi_buf rx_bufs[2];
    struct spi_buf_set tx = { .buffers = tx_bufs, .count = 2 };
    struct spi_buf_set rx = { .buffers = rx_bufs, .count = 2 };

    tx_bufs[0].buf = tx_buf;
    tx_bufs[0].len = 4;
    rx_bufs[0].buf = rx_buf;
    rx_bufs[0].len = 4;

    tx_bufs[1].buf = NULL;
    tx_bufs[1].len = len;
    rx_bufs[1].buf = buf;
    rx_bufs[1].len = len;

    ret = spi_transceive_dt(spec, &tx, &rx);
    if (ret < 0) {
        return ret;
    }
    return len;
}

int w25q64jv_write_page(const struct device *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    const struct spi_dt_spec *spec = (const struct spi_dt_spec *)dev;
    uint8_t cmd_wren = 0x06;
    uint8_t cmd_pp[4];
    int ret;

    if (len == 0 || len > 256) {
        return -EINVAL;
    }

    /* Write enable */
    ret = spi_write_then_read(spec, &cmd_wren, 1, NULL, 0);
    if (ret < 0) {
        return ret;
    }

    /* Page program command + address + data */
    cmd_pp[0] = 0x02;
    cmd_pp[1] = (addr >> 16) & 0xFF;
    cmd_pp[2] = (addr >> 8) & 0xFF;
    cmd_pp[3] = addr & 0xFF;

    struct spi_buf tx_bufs[2];
    struct spi_buf_set tx = { .buffers = tx_bufs, .count = 2 };

    tx_bufs[0].buf = cmd_pp;
    tx_bufs[0].len = 4;
    tx_bufs[1].buf = (void *)buf;
    tx_bufs[1].len = len;

    ret = spi_transceive_dt(spec, &tx, NULL);
    if (ret < 0) {
        return ret;
    }
    return len;
}
