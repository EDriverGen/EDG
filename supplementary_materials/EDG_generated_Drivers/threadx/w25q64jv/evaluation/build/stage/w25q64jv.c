#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include "tx_api.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "threadx.h"
#define W25Q64JV_CMD_READ_DATA        0x03u
#define W25Q64JV_CMD_PAGE_PROGRAM     0x02u
#define W25Q64JV_CMD_WRITE_ENABLE     0x06u
#define W25Q64JV_CMD_READ_STATUS_1    0x05u
#define W25Q64JV_CMD_JEDEC_ID         0x9Fu

#define W25Q64JV_PAGE_SIZE            256u
#define W25Q64JV_SPI_TIMEOUT_MS       1000u
#define W25Q64JV_BUSY_POLL_LIMIT      100000u

static int w25q64jv_spi_xfer(w25q64jv_dev_t *dev, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef st;

    if (hspi == NULL) {
        return -1;
    }

    st = HAL_SPI_TransmitReceive(hspi, tx, rx, len, W25Q64JV_SPI_TIMEOUT_MS);
    if (st != HAL_OK) {
        return -1;
    }
    return 0;
}

static int w25q64jv_write_cmd(w25q64jv_dev_t *dev, uint8_t cmd)
{
    uint8_t tx[1];
    uint8_t rx[1];

    tx[0] = cmd;
    rx[0] = 0u;
    return w25q64jv_spi_xfer(dev, tx, rx, 1u);
}

static int w25q64jv_read_status1(w25q64jv_dev_t *dev, uint8_t *status)
{
    uint8_t tx[2];
    uint8_t rx[2];

    tx[0] = W25Q64JV_CMD_READ_STATUS_1;
    tx[1] = 0x00u;
    rx[0] = 0u;
    rx[1] = 0u;

    if (w25q64jv_spi_xfer(dev, tx, rx, 2u) != 0) {
        return -1;
    }
    *status = rx[1];
    return 0;
}

static int w25q64jv_wait_ready(w25q64jv_dev_t *dev)
{
    uint32_t i;
    uint8_t status = 0u;

    for (i = 0u; i < W25Q64JV_BUSY_POLL_LIMIT; i++) {
        if (w25q64jv_read_status1(dev, &status) != 0) {
            return -1;
        }
        if ((status & 0x01u) == 0u) {
            return 0;
        }
    }
    return -1;
}

int w25q64jv_init(w25q64jv_dev_t *dev, void *bus_handle)
{
    uint8_t tx[4];
    uint8_t rx[4];

    if (dev == NULL) {
        return -1;
    }

    dev->bus_handle = bus_handle;
    dev->i2c_addr = 0x00u;

    if (bus_handle == NULL) {
        return -1;
    }

    tx[0] = W25Q64JV_CMD_JEDEC_ID;
    tx[1] = 0x00u;
    tx[2] = 0x00u;
    tx[3] = 0x00u;
    rx[0] = 0u;
    rx[1] = 0u;
    rx[2] = 0u;
    rx[3] = 0u;

    if (w25q64jv_spi_xfer(dev, tx, rx, 4u) != 0) {
        return -1;
    }

    return 0;
}

int w25q64jv_read(w25q64jv_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t cmd[4];
    uint8_t dummy[4];
    uint32_t remaining;
    uint32_t offset;

    if (dev == NULL || buf == NULL) {
        return -1;
    }
    if (len == 0u) {
        return 0;
    }

    if (w25q64jv_wait_ready(dev) != 0) {
        return -1;
    }

    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (uint8_t)((addr >> 16) & 0xFFu);
    cmd[2] = (uint8_t)((addr >> 8) & 0xFFu);
    cmd[3] = (uint8_t)(addr & 0xFFu);
    dummy[0] = 0u;
    dummy[1] = 0u;
    dummy[2] = 0u;
    dummy[3] = 0u;

    if (w25q64jv_spi_xfer(dev, cmd, dummy, 4u) != 0) {
        return -1;
    }

    remaining = len;
    offset = 0u;
    while (remaining > 0u) {
        uint16_t chunk = (remaining > 256u) ? (uint16_t)256u : (uint16_t)remaining;
        uint16_t i;
        uint8_t txbuf[256];
        uint8_t rxbuf[256];

        for (i = 0u; i < chunk; i++) {
            txbuf[i] = 0x00u;
            rxbuf[i] = 0x00u;
        }

        if (w25q64jv_spi_xfer(dev, txbuf, rxbuf, chunk) != 0) {
            return -1;
        }

        for (i = 0u; i < chunk; i++) {
            buf[offset + i] = rxbuf[i];
        }

        offset += (uint32_t)chunk;
        remaining -= (uint32_t)chunk;
    }

    return 0;
}

int w25q64jv_write(w25q64jv_dev_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t remaining;
    uint32_t offset;

    if (dev == NULL || buf == NULL) {
        return -1;
    }
    if (len == 0u) {
        return 0;
    }

    remaining = len;
    offset = 0u;
    while (remaining > 0u) {
        uint32_t page_off = (addr + offset) & (W25Q64JV_PAGE_SIZE - 1u);
        uint32_t page_space = W25Q64JV_PAGE_SIZE - page_off;
        uint32_t chunk = (remaining < page_space) ? remaining : page_space;
        uint8_t txbuf[4u + W25Q64JV_PAGE_SIZE];
        uint8_t rxbuf[4u + W25Q64JV_PAGE_SIZE];
        uint32_t i;
        uint32_t total = 4u + chunk;
        uint32_t cur_addr = addr + offset;

        if (w25q64jv_wait_ready(dev) != 0) {
            return -1;
        }

        if (w25q64jv_write_cmd(dev, W25Q64JV_CMD_WRITE_ENABLE) != 0) {
            return -1;
        }

        txbuf[0] = W25Q64JV_CMD_PAGE_PROGRAM;
        txbuf[1] = (uint8_t)((cur_addr >> 16) & 0xFFu);
        txbuf[2] = (uint8_t)((cur_addr >> 8) & 0xFFu);
        txbuf[3] = (uint8_t)(cur_addr & 0xFFu);
        for (i = 0u; i < chunk; i++) {
            txbuf[4u + i] = buf[offset + i];
        }
        for (i = 0u; i < total; i++) {
            rxbuf[i] = 0x00u;
        }

        if (w25q64jv_spi_xfer(dev, txbuf, rxbuf, (uint16_t)total) != 0) {
            return -1;
        }

        if (w25q64jv_wait_ready(dev) != 0) {
            return -1;
        }

        offset += chunk;
        remaining -= chunk;
    }

    return 0;
}
