#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "cmsis_rtx.h"
#define W25Q64JV_CMD_READ_DATA 0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID 0x9F

#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_TIMEOUT 1000

static int w25q64jv_transfer(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, uint16_t size)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, tx, rx, size, W25Q64JV_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t tx[4] = {W25Q64JV_CMD_JEDEC_ID, 0, 0, 0};
    uint8_t rx[4] = {0};
    int ret = w25q64jv_transfer(dev, tx, rx, 4);
    if (ret != 0) return -1;
    // Expected JEDEC ID: 0xEF 0x40 0x17 for W25Q64JV
    if (rx[1] != 0xEF || rx[2] != 0x40 || rx[3] != 0x17) return -1;
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t tx[4];
    tx[0] = W25Q64JV_CMD_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    uint8_t rx[4];
    int ret = w25q64jv_transfer(dev, tx, rx, 4);
    if (ret != 0) return -1;
    // Now read data bytes
    size_t remaining = len;
    while (remaining > 0) {
        uint16_t chunk = (remaining > W25Q64JV_PAGE_SIZE) ? W25Q64JV_PAGE_SIZE : (uint16_t)remaining;
        ret = w25q64jv_transfer(dev, NULL, buf, chunk);
        if (ret != 0) return -1;
        buf += chunk;
        remaining -= chunk;
    }
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    // Write enable
    uint8_t we_cmd = W25Q64JV_CMD_WRITE_ENABLE;
    int ret = w25q64jv_transfer(dev, &we_cmd, NULL, 1);
    if (ret != 0) return -1;
    // Page program
    uint8_t tx[4 + W25Q64JV_PAGE_SIZE];
    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    size_t chunk = (len > W25Q64JV_PAGE_SIZE) ? W25Q64JV_PAGE_SIZE : len;
    for (size_t i = 0; i < chunk; i++) {
        tx[4 + i] = buf[i];
    }
    ret = w25q64jv_transfer(dev, tx, NULL, 4 + chunk);
    if (ret != 0) return -1;
    // Wait for completion (poll BUSY)
    uint8_t status_cmd = 0x05;
    uint8_t status_rx;
    do {
        ret = w25q64jv_transfer(dev, &status_cmd, &status_rx, 1);
        if (ret != 0) return -1;
        // status_rx is the status register; BUSY is bit 0
    } while (status_rx & 0x01);
    return 0;
}
