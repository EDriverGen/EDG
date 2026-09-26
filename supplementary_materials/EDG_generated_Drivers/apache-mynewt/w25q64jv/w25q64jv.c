#include "w25q64jv.h"
#include <assert.h>
#include <string.h>

#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

static int spi_write_then_read(struct w25q64jv_dev *dev, const uint8_t *txbuf, size_t txlen, uint8_t *rxbuf, size_t rxlen)
{
    int rc;
    struct hal_spi_settings spi_settings;
    int spi_num = 0; /* from fixed_attachment */
    /* Assume bus_handle is an int representing SPI interface number */
    int spi_handle = (int)(uintptr_t)dev->bus_handle;
    /* Use hal_spi interface */
    /* For simplicity, we use hal_spi_txrx */
    /* We need to combine tx and rx into a single transaction */
    size_t total = txlen + rxlen;
    uint8_t *full_tx = malloc(total);
    uint8_t *full_rx = malloc(total);
    if (!full_tx || !full_rx) {
        free(full_tx);
        free(full_rx);
        return -1;
    }
    memcpy(full_tx, txbuf, txlen);
    memset(full_tx + txlen, 0, rxlen);
    rc = hal_spi_txrx(spi_handle, full_tx, full_rx, total);
    if (rc == 0 && rxbuf) {
        memcpy(rxbuf, full_rx + txlen, rxlen);
    }
    free(full_tx);
    free(full_rx);
    return rc;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    int rc;
    uint8_t txbuf[1] = {W25Q64JV_CMD_JEDEC_ID};
    uint8_t rxbuf[3];
    dev->bus_handle = bus_handle;
    /* JEDEC ID read */
    rc = spi_write_then_read(dev, txbuf, 1, rxbuf, 3);
    if (rc != 0) return rc;
    /* Optionally verify JEDEC ID: 0xEF 0x40 0x17 */
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    int rc;
    uint8_t txbuf[4];
    txbuf[0] = W25Q64JV_CMD_READ_DATA;
    txbuf[1] = (addr >> 16) & 0xFF;
    txbuf[2] = (addr >> 8) & 0xFF;
    txbuf[3] = addr & 0xFF;
    rc = spi_write_then_read(dev, txbuf, 4, buf, len);
    return rc;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    int rc;
    uint8_t txbuf[4];
    uint8_t *full_tx;
    size_t total;
    /* Write enable */
    uint8_t we_cmd = W25Q64JV_CMD_WRITE_ENABLE;
    rc = spi_write_then_read(dev, &we_cmd, 1, NULL, 0);
    if (rc != 0) return rc;
    /* Page program */
    total = 4 + len;
    full_tx = malloc(total);
    if (!full_tx) return -1;
    full_tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    full_tx[1] = (addr >> 16) & 0xFF;
    full_tx[2] = (addr >> 8) & 0xFF;
    full_tx[3] = addr & 0xFF;
    memcpy(full_tx + 4, buf, len);
    rc = spi_write_then_read(dev, full_tx, total, NULL, 0);
    free(full_tx);
    return rc;
}