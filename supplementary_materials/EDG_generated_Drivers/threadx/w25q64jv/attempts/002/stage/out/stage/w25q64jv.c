#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include "tx_api.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "threadx.h"
#define W25Q64JV_CMD_READ_DATA        0x03u
#define W25Q64JV_CMD_PAGE_PROGRAM     0x02u
#define W25Q64JV_CMD_SECTOR_ERASE_4K  0x20u
#define W25Q64JV_CMD_WRITE_ENABLE     0x06u
#define W25Q64JV_CMD_READ_STATUS_1    0x05u
#define W25Q64JV_CMD_JEDEC_ID         0x9Fu

#define W25Q64JV_STATUS_BUSY          0x01u
#define W25Q64JV_STATUS_WEL           0x02u

#define W25Q64JV_PAGE_BYTES           256u
#define W25Q64JV_SPI_TIMEOUT_MS       1000u
#define W25Q64JV_READY_TIMEOUT_MS     5000u

static int w25q64jv_spi_xfer(w25q64jv_dev_t *dev,
                             const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    SPI_HandleTypeDef *hspi;
    HAL_StatusTypeDef st;

    if (dev == NULL || dev->bus_handle == NULL) {
        return -1;
    }
    if (len == 0u) {
        return 0;
    }

    hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    st = HAL_SPI_TransmitReceive(hspi, tx, rx, len, W25Q64JV_SPI_TIMEOUT_MS);
    if (st != HAL_OK) {
        return -1;
    }
    return 0;
}

static int w25q64jv_write_cmd(w25q64jv_dev_t *dev, uint8_t cmd)
{
    uint8_t tx[1];
    tx[0] = cmd;
    return w25q64jv_spi_xfer(dev, tx, NULL, 1u);
}

static int w25q64jv_read_status1_internal(w25q64jv_dev_t *dev, uint8_t *status)
{
    uint8_t tx[2];
    uint8_t rx[2];
    int rc;

    if (status == NULL) {
        return -1;
    }

    tx[0] = W25Q64JV_CMD_READ_STATUS_1;
    tx[1] = 0x00u;
    rx[0] = 0x00u;
    rx[1] = 0x00u;

    rc = w25q64jv_spi_xfer(dev, tx, rx, 2u);
    if (rc != 0) {
        return rc;
    }
    *status = rx[1];
    return 0;
}

int w25q64jv_read_status1(w25q64jv_dev_t *dev, uint8_t *status)
{
    return w25q64jv_read_status1_internal(dev, status);
}

int w25q64jv_wait_ready(w25q64jv_dev_t *dev, uint32_t timeout_ms)
{
    uint32_t elapsed = 0u;
    uint8_t status = 0u;
    int rc;

    for (;;) {
        rc = w25q64jv_read_status1_internal(dev, &status);
        if (rc != 0) {
            return rc;
        }
        if ((status & W25Q64JV_STATUS_BUSY) == 0u) {
            return 0;
        }
        if (elapsed >= timeout_ms) {
            return -1;
        }
        HAL_Delay(1u);
        elapsed += 1u;
    }
}

int w25q64jv_read_jedec_id(w25q64jv_dev_t *dev, uint8_t *mfr, uint8_t *mem_type, uint8_t *capacity)
{
    uint8_t tx[4];
    uint8_t rx[4];
    int rc;

    if (mfr == NULL || mem_type == NULL || capacity == NULL) {
        return -1;
    }

    tx[0] = W25Q64JV_CMD_JEDEC_ID;
    tx[1] = 0x00u;
    tx[2] = 0x00u;
    tx[3] = 0x00u;
    rx[0] = 0x00u;
    rx[1] = 0x00u;
    rx[2] = 0x00u;
    rx[3] = 0x00u;

    rc = w25q64jv_spi_xfer(dev, tx, rx, 4u);
    if (rc != 0) {
        return rc;
    }

    *mfr = rx[1];
    *mem_type = rx[2];
    *capacity = rx[3];
    return 0;
}

int w25q64jv_init(w25q64jv_dev_t *dev, void *bus_handle)
{
    uint8_t mfr = 0u;
    uint8_t mem_type = 0u;
    uint8_t capacity = 0u;
    int rc;

    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }

    dev->bus_handle = bus_handle;
    dev->jedec_mfr = 0u;
    dev->jedec_mem_type = 0u;
    dev->jedec_capacity = 0u;
    dev->initialized = 0u;

    rc = w25q64jv_read_jedec_id(dev, &mfr, &mem_type, &capacity);
    if (rc != 0) {
        return rc;
    }

    dev->jedec_mfr = mfr;
    dev->jedec_mem_type = mem_type;
    dev->jedec_capacity = capacity;
    dev->initialized = 1u;
    return 0;
}

int w25q64jv_read(w25q64jv_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t cmd[4];
    uint8_t dummy[4];
    uint32_t remaining;
    uint32_t offset;
    int rc;

    if (dev == NULL || buf == NULL) {
        return -1;
    }
    if (len == 0u) {
        return 0;
    }

    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (uint8_t)((addr >> 16) & 0xFFu);
    cmd[2] = (uint8_t)((addr >> 8) & 0xFFu);
    cmd[3] = (uint8_t)(addr & 0xFFu);
    dummy[0] = 0x00u;
    dummy[1] = 0x00u;
    dummy[2] = 0x00u;
    dummy[3] = 0x00u;

    rc = w25q64jv_spi_xfer(dev, cmd, dummy, 4u);
    if (rc != 0) {
        return rc;
    }

    remaining = len;
    offset = 0u;
    while (remaining > 0u) {
        uint16_t chunk = (remaining > 0xFFFFu) ? 0xFFFFu : (uint16_t)remaining;
        uint16_t i;
        for (i = 0u; i < chunk; i++) {
            dummy[i & 0x3u] = 0x00u;
        }
        rc = w25q64jv_spi_xfer(dev, dummy, &buf[offset], chunk);
        if (rc != 0) {
            return rc;
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
    int rc;

    if (dev == NULL || buf == NULL) {
        return -1;
    }
    if (len == 0u) {
        return 0;
    }

    remaining = len;
    offset = 0u;

    while (remaining > 0u) {
        uint32_t page_off = addr & (W25Q64JV_PAGE_BYTES - 1u);
        uint32_t page_space = W25Q64JV_PAGE_BYTES - page_off;
        uint32_t chunk = (remaining < page_space) ? remaining : page_space;
        uint8_t hdr[4];
        uint8_t dummy[4];
        uint8_t status = 0u;

        rc = w25q64jv_write_cmd(dev, W25Q64JV_CMD_WRITE_ENABLE);
        if (rc != 0) {
            return rc;
        }

        rc = w25q64jv_read_status1_internal(dev, &status);
        if (rc != 0) {
            return rc;
        }
        if ((status & W25Q64JV_STATUS_WEL) == 0u) {
            return -1;
        }

        hdr[0] = W25Q64JV_CMD_PAGE_PROGRAM;
        hdr[1] = (uint8_t)((addr >> 16) & 0xFFu);
        hdr[2] = (uint8_t)((addr >> 8) & 0xFFu);
        hdr[3] = (uint8_t)(addr & 0xFFu);
        dummy[0] = 0x00u;
        dummy[1] = 0x00u;
        dummy[2] = 0x00u;
        dummy[3] = 0x00u;

        rc = w25q64jv_spi_xfer(dev, hdr, dummy, 4u);
        if (rc != 0) {
            return rc;
        }

        {
            uint32_t sent = 0u;
            while (sent < chunk) {
                uint32_t sub = chunk - sent;
                uint16_t sub16;
                if (sub > 0xFFFFu) {
                    sub = 0xFFFFu;
                }
                sub16 = (uint16_t)sub;
                rc = w25q64jv_spi_xfer(dev, &buf[offset + sent], NULL, sub16);
                if (rc != 0) {
                    return rc;
                }
                sent += (uint32_t)sub16;
            }
        }

        rc = w25q64jv_wait_ready(dev, W25Q64JV_READY_TIMEOUT_MS);
        if (rc != 0) {
            return rc;
        }

        offset += chunk;
        remaining -= chunk;
        addr += chunk;
    }

    return 0;
}
