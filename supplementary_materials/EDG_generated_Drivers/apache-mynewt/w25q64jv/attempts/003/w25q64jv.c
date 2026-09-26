#include "w25q64jv.h"
#include <assert.h>
#include <string.h>

#include <os/os_time.h>
#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_READ_STATUS_1  0x05
#define W25Q64JV_CMD_JEDEC_ID       0x9F

#define W25Q64JV_DUMMY_BYTE         0x00

static int w25q64jv_spi_write_then_read(struct w25q64jv_dev *dev, const uint8_t *txbuf, size_t txlen, uint8_t *rxbuf, size_t rxlen)
{
    int rc;
    struct hal_spi_settings spi_settings;
    uint8_t *full_tx = NULL;
    uint8_t *full_rx = NULL;
    size_t total = txlen + rxlen;

    if (total == 0) return 0;

    full_tx = (uint8_t *)malloc(total);
    full_rx = (uint8_t *)malloc(total);
    if (!full_tx || !full_rx) {
        free(full_tx);
        free(full_rx);
        return -1;
    }

    memcpy(full_tx, txbuf, txlen);
    memset(full_tx + txlen, W25Q64JV_DUMMY_BYTE, rxlen);

    rc = hal_spi_txrx(dev->bus_handle, full_tx, full_rx, total);
    if (rc == 0 && rxbuf && rxlen > 0) {
        memcpy(rxbuf, full_rx + txlen, rxlen);
    }

    free(full_tx);
    free(full_rx);
    return rc;
}

static int w25q64jv_spi_write(struct w25q64jv_dev *dev, const uint8_t *txbuf, size_t len)
{
    return w25q64jv_spi_write_then_read(dev, txbuf, len, NULL, 0);
}

static int w25q64jv_poll_busy(struct w25q64jv_dev *dev)
{
    uint8_t cmd = W25Q64JV_CMD_READ_STATUS_1;
    uint8_t status;
    int timeout = 1000;

    do {
        if (w25q64jv_spi_write_then_read(dev, &cmd, 1, &status, 1) != 0)
            return -1;
        if (!(status & 0x01))
            return 0;
        os_time_delay(1);
    } while (--timeout > 0);

    return -1;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t jedec[3];
    int rc;

    dev->bus_handle = bus_handle;
    dev->cs_pin = 0; /* CS is managed by HAL SPI */

    rc = w25q64jv_spi_write_then_read(dev, &cmd, 1, jedec, 3);
    if (rc != 0)
        return rc;

    /* Optionally verify JEDEC ID: 0xEF 0x40 0x17 for W25Q64JV */
    if (jedec[0] != 0xEF || jedec[1] != 0x40 || jedec[2] != 0x17)
        return -1;

    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    int rc;

    if (len == 0 || buf == NULL)
        return -1;

    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    rc = w25q64jv_spi_write_then_read(dev, cmd, 4, buf, len);
    return rc;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    uint8_t *txbuf;
    int rc;

    if (len == 0 || len > 256 || buf == NULL)
        return -1;

    /* Write enable */
    uint8_t we_cmd = W25Q64JV_CMD_WRITE_ENABLE;
    rc = w25q64jv_spi_write(dev, &we_cmd, 1);
    if (rc != 0)
        return rc;

    /* Page program command + address + data */
    cmd[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    txbuf = (uint8_t *)malloc(4 + len);
    if (!txbuf)
        return -1;
    memcpy(txbuf, cmd, 4);
    memcpy(txbuf + 4, buf, len);

    rc = w25q64jv_spi_write(dev, txbuf, 4 + len);
    free(txbuf);
    if (rc != 0)
        return rc;

    /* Poll BUSY */
    rc = w25q64jv_poll_busy(dev);
    return rc;
}
