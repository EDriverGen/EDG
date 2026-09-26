#include "w25q64jv.h"
#include <assert.h>
#include <string.h>

#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

static int w25q64jv_spi_transfer(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    int rc = hal_spi_txrx(dev->bus_handle, dev->cs_pin, tx, rx, len);
    return rc;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->cs_pin = 0; /* board_configured_cs */

    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t rx[3];
    int rc = w25q64jv_spi_transfer(dev, &cmd, rx, 1);
    if (rc != 0) return rc;
    rc = w25q64jv_spi_transfer(dev, NULL, rx, 3);
    if (rc != 0) return rc;

    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    int rc = w25q64jv_spi_transfer(dev, cmd, NULL, 4);
    if (rc != 0) return rc;

    rc = w25q64jv_spi_transfer(dev, NULL, buf, len);
    return rc;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0 || len > 256) return -1;

    uint8_t cmd_wren = W25Q64JV_CMD_WRITE_ENABLE;
    int rc = w25q64jv_spi_transfer(dev, &cmd_wren, NULL, 1);
    if (rc != 0) return rc;

    uint8_t cmd_page[4];
    cmd_page[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    cmd_page[1] = (addr >> 16) & 0xFF;
    cmd_page[2] = (addr >> 8) & 0xFF;
    cmd_page[3] = addr & 0xFF;

    rc = w25q64jv_spi_transfer(dev, cmd_page, NULL, 4);
    if (rc != 0) return rc;

    rc = w25q64jv_spi_transfer(dev, buf, NULL, len);
    return rc;
}