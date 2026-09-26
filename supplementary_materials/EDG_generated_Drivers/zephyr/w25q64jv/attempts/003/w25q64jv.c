#include "w25q64jv.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
#define CMD_READ_DATA 0x03
#define CMD_PAGE_PROGRAM 0x02
#define CMD_WRITE_ENABLE 0x06
#define CMD_JEDEC_ID 0x9F

static int spi_write_then_read(const struct spi_dt_spec *spec,
                                const uint8_t *tx_data, size_t tx_len,
                                uint8_t *rx_data, size_t rx_len)
{
    struct spi_buf tx_buf = { .buf = (void *)tx_data, .len = tx_len };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
    struct spi_buf rx_buf = { .buf = rx_data, .len = rx_len };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    return spi_transceive_dt(spec, &tx, &rx);
}

int w25q64jv_init(const struct spi_dt_spec *dev, const struct spi_dt_spec *cfg)
{
    uint8_t cmd = CMD_JEDEC_ID;
    uint8_t jedec[3];
    int ret;

    ret = spi_write_then_read(cfg, &cmd, 1, jedec, 3);
    if (ret < 0) {
        return ret;
    }
    /* Expected JEDEC ID: 0xEF 0x40 0x17 for W25Q64JV */
    if (jedec[0] != 0xEF || jedec[1] != 0x40 || jedec[2] != 0x17) {
        return -ENODEV;
    }
    return 0;
}

int w25q64jv_read(const struct spi_dt_spec *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t tx_buf[4];
    uint8_t rx_buf[4];
    int ret;

    tx_buf[0] = CMD_READ_DATA;
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

    ret = spi_transceive_dt(dev, &tx, &rx);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int w25q64jv_write_page(const struct spi_dt_spec *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    uint8_t cmd_enable = CMD_WRITE_ENABLE;
    uint8_t tx_prog[4 + 256];
    int ret;

    if (len > 256) {
        len = 256;
    }

    /* Write enable */
    ret = spi_write_then_read(dev, &cmd_enable, 1, NULL, 0);
    if (ret < 0) {
        return ret;
    }

    /* Page program */
    tx_prog[0] = CMD_PAGE_PROGRAM;
    tx_prog[1] = (addr >> 16) & 0xFF;
    tx_prog[2] = (addr >> 8) & 0xFF;
    tx_prog[3] = addr & 0xFF;
    for (size_t i = 0; i < len; i++) {
        tx_prog[4 + i] = buf[i];
    }

    ret = spi_write_then_read(dev, tx_prog, 4 + len, NULL, 0);
    if (ret < 0) {
        return ret;
    }

    /* Wait for BUSY to clear (poll status register) */
    uint8_t status_cmd = 0x05;
    uint8_t status;
    do {
        k_sleep(K_MSEC(1));
        ret = spi_write_then_read(dev, &status_cmd, 1, &status, 1);
        if (ret < 0) {
            return ret;
        }
    } while (status & 0x01);

    return 0;
}
