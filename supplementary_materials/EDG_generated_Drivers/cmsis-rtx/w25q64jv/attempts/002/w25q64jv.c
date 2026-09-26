#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "cmsis_rtx.h"
#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

#define W25Q64JV_PAGE_SIZE          256
#define W25Q64JV_TIMEOUT            1000

static int w25q64jv_transfer(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, uint16_t size)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    if (HAL_SPI_TransmitReceive(hspi, (uint8_t *)tx, rx, size, W25Q64JV_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t tx[1] = {W25Q64JV_CMD_JEDEC_ID};
    uint8_t rx[4] = {0};
    if (w25q64jv_transfer(dev, tx, rx, 1) != 0)
        return -1;
    if (w25q64jv_transfer(dev, tx, rx, 3) != 0)
        return -1;
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
    if (w25q64jv_transfer(dev, cmd, NULL, 4) != 0)
        return -1;
    uint8_t dummy[W25Q64JV_PAGE_SIZE];
    if (w25q64jv_transfer(dev, dummy, buf, (uint16_t)len) != 0)
        return -1;
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t enable_cmd[1] = {W25Q64JV_CMD_WRITE_ENABLE};
    if (w25q64jv_transfer(dev, enable_cmd, NULL, 1) != 0)
        return -1;
    uint8_t cmd[4 + W25Q64JV_PAGE_SIZE];
    cmd[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    size_t copy_len = len > W25Q64JV_PAGE_SIZE ? W25Q64JV_PAGE_SIZE : len;
    for (size_t i = 0; i < copy_len; i++)
        cmd[4 + i] = buf[i];
    if (w25q64jv_transfer(dev, cmd, NULL, (uint16_t)(4 + copy_len)) != 0)
        return -1;
    return 0;
}
