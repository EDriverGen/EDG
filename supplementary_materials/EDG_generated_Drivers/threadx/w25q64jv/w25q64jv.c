#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include "tx_api.h"
#include <string.h>

#include "threadx.h"
#define W25Q64JV_SPI_HANDLE(dev) ((SPI_HandleTypeDef *)((dev)->bus_handle))

static int w25q64jv_spi_xfer(w25q64jv_dev_t *dev, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    SPI_HandleTypeDef *hspi;
    HAL_StatusTypeDef st;

    if (dev == NULL || dev->bus_handle == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    hspi = W25Q64JV_SPI_HANDLE(dev);
    st = HAL_SPI_TransmitReceive(hspi, tx, rx, len, W25Q64JV_SPI_TIMEOUT_MS);
    if (st != HAL_OK) {
        return W25Q64JV_ERR_IO;
    }
    return W25Q64JV_OK;
}

static int w25q64jv_cmd_only(w25q64jv_dev_t *dev, uint8_t cmd)
{
    uint8_t tx[1];
    uint8_t rx[1];

    tx[0] = cmd;
    rx[0] = 0u;
    return w25q64jv_spi_xfer(dev, tx, rx, 1u);
}

int w25q64jv_read_status(w25q64jv_dev_t *dev, uint8_t *status)
{
    uint8_t tx[2];
    uint8_t rx[2];
    int rc;

    if (dev == NULL || status == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    tx[0] = W25Q64JV_CMD_READ_STATUS_1;
    tx[1] = 0x00u;
    rx[0] = 0u;
    rx[1] = 0u;
    rc = w25q64jv_spi_xfer(dev, tx, rx, 2u);
    if (rc != W25Q64JV_OK) {
        return rc;
    }
    *status = rx[1];
    return W25Q64JV_OK;
}

int w25q64jv_wait_ready(w25q64jv_dev_t *dev, uint32_t timeout_ms)
{
    uint32_t elapsed = 0u;
    uint8_t status = 0u;
    int rc;

    if (dev == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    for (;;) {
        rc = w25q64jv_read_status(dev, &status);
        if (rc != W25Q64JV_OK) {
            return rc;
        }
        if ((status & W25Q64JV_STATUS_BUSY) == 0u) {
            return W25Q64JV_OK;
        }
        if (elapsed >= timeout_ms) {
            return W25Q64JV_ERR_TIMEOUT;
        }
        HAL_Delay(1u);
        elapsed += 1u;
    }
}

int w25q64jv_write_enable(w25q64jv_dev_t *dev)
{
    uint8_t status = 0u;
    int rc;

    if (dev == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    rc = w25q64jv_cmd_only(dev, W25Q64JV_CMD_WRITE_ENABLE);
    if (rc != W25Q64JV_OK) {
        return rc;
    }
    rc = w25q64jv_read_status(dev, &status);
    if (rc != W25Q64JV_OK) {
        return rc;
    }
    if ((status & W25Q64JV_STATUS_WEL) == 0u) {
        return W25Q64JV_ERR_PERM;
    }
    return W25Q64JV_OK;
}

int w25q64jv_read_jedec_id(w25q64jv_dev_t *dev, uint8_t *mfr, uint8_t *device, uint8_t *capacity)
{
    uint8_t tx[4];
    uint8_t rx[4];
    int rc;

    if (dev == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    tx[0] = W25Q64JV_CMD_JEDEC_ID;
    tx[1] = 0x00u;
    tx[2] = 0x00u;
    tx[3] = 0x00u;
    rx[0] = 0u;
    rx[1] = 0u;
    rx[2] = 0u;
    rx[3] = 0u;
    rc = w25q64jv_spi_xfer(dev, tx, rx, 4u);
    if (rc != W25Q64JV_OK) {
        return rc;
    }
    dev->jedec_mfr = rx[1];
    dev->jedec_device = rx[2];
    dev->jedec_capacity = rx[3];
    if (mfr != NULL) {
        *mfr = rx[1];
    }
    if (device != NULL) {
        *device = rx[2];
    }
    if (capacity != NULL) {
        *capacity = rx[3];
    }
    return W25Q64JV_OK;
}

int w25q64jv_init(w25q64jv_dev_t *dev, void *bus_handle)
{
    int rc;

    if (dev == NULL || bus_handle == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    dev->bus_handle = bus_handle;
    dev->jedec_mfr = 0u;
    dev->jedec_device = 0u;
    dev->jedec_capacity = 0u;
    dev->initialized = 0u;

    rc = w25q64jv_read_jedec_id(dev, NULL, NULL, NULL);
    if (rc != W25Q64JV_OK) {
        return rc;
    }
    dev->initialized = 1u;
    return W25Q64JV_OK;
}

int w25q64jv_read(w25q64jv_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t cmd[4];
    uint8_t dummy[4];
    uint32_t remaining;
    uint32_t offset;
    int rc;

    if (dev == NULL || buf == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    if (len == 0u) {
        return W25Q64JV_OK;
    }
    if (addr >= W25Q64JV_MEMORY_SIZE || (addr + len) > W25Q64JV_MEMORY_SIZE) {
        return W25Q64JV_ERR_PARAM;
    }

    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (uint8_t)((addr >> 16) & 0xFFu);
    cmd[2] = (uint8_t)((addr >> 8) & 0xFFu);
    cmd[3] = (uint8_t)(addr & 0xFFu);
    dummy[0] = 0u;
    dummy[1] = 0u;
    dummy[2] = 0u;
    dummy[3] = 0u;

    rc = w25q64jv_spi_xfer(dev, cmd, dummy, 4u);
    if (rc != W25Q64JV_OK) {
        return rc;
    }

    remaining = len;
    offset = 0u;
    while (remaining > 0u) {
        uint16_t chunk = (remaining > 0xFFFFu) ? 0xFFFFu : (uint16_t)remaining;
        uint16_t i;
        for (i = 0u; i < chunk; i++) {
            cmd[i & 3u] = 0x00u;
        }
        rc = w25q64jv_spi_xfer(dev, cmd, &buf[offset], chunk);
        if (rc != W25Q64JV_OK) {
            return rc;
        }
        offset += (uint32_t)chunk;
        remaining -= (uint32_t)chunk;
    }
    return W25Q64JV_OK;
}

int w25q64jv_write(w25q64jv_dev_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t remaining;
    uint32_t offset;
    int rc;

    if (dev == NULL || buf == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    if (len == 0u) {
        return W25Q64JV_OK;
    }
    if (addr >= W25Q64JV_MEMORY_SIZE || (addr + len) > W25Q64JV_MEMORY_SIZE) {
        return W25Q64JV_ERR_PARAM;
    }

    remaining = len;
    offset = 0u;
    while (remaining > 0u) {
        uint32_t page_off = (addr + offset) % W25Q64JV_PAGE_SIZE;
        uint32_t page_room = W25Q64JV_PAGE_SIZE - page_off;
        uint32_t chunk = (remaining < page_room) ? remaining : page_room;
        uint8_t frame[4u + W25Q64JV_PAGE_SIZE];
        uint8_t rx[4u + W25Q64JV_PAGE_SIZE];
        uint32_t i;

        rc = w25q64jv_write_enable(dev);
        if (rc != W25Q64JV_OK) {
            return rc;
        }

        frame[0] = W25Q64JV_CMD_PAGE_PROGRAM;
        frame[1] = (uint8_t)(((addr + offset) >> 16) & 0xFFu);
        frame[2] = (uint8_t)(((addr + offset) >> 8) & 0xFFu);
        frame[3] = (uint8_t)((addr + offset) & 0xFFu);
        for (i = 0u; i < chunk; i++) {
            frame[4u + i] = buf[offset + i];
        }
        for (i = 0u; i < (4u + chunk); i++) {
            rx[i] = 0u;
        }

        rc = w25q64jv_spi_xfer(dev, frame, rx, (uint16_t)(4u + chunk));
        if (rc != W25Q64JV_OK) {
            return rc;
        }

        rc = w25q64jv_wait_ready(dev, W25Q64JV_PROGRAM_TIMEOUT_MS);
        if (rc != W25Q64JV_OK) {
            return rc;
        }

        offset += chunk;
        remaining -= chunk;
    }
    return W25Q64JV_OK;
}

int w25q64jv_erase_sector(w25q64jv_dev_t *dev, uint32_t addr)
{
    uint8_t frame[4];
    uint8_t rx[4];
    int rc;

    if (dev == NULL) {
        return W25Q64JV_ERR_PARAM;
    }
    if (addr >= W25Q64JV_MEMORY_SIZE) {
        return W25Q64JV_ERR_PARAM;
    }

    rc = w25q64jv_write_enable(dev);
    if (rc != W25Q64JV_OK) {
        return rc;
    }

    frame[0] = W25Q64JV_CMD_SECTOR_ERASE_4K;
    frame[1] = (uint8_t)((addr >> 16) & 0xFFu);
    frame[2] = (uint8_t)((addr >> 8) & 0xFFu);
    frame[3] = (uint8_t)(addr & 0xFFu);
    rx[0] = 0u;
    rx[1] = 0u;
    rx[2] = 0u;
    rx[3] = 0u;

    rc = w25q64jv_spi_xfer(dev, frame, rx, 4u);
    if (rc != W25Q64JV_OK) {
        return rc;
    }

    return w25q64jv_wait_ready(dev, W25Q64JV_ERASE_TIMEOUT_MS);
}
