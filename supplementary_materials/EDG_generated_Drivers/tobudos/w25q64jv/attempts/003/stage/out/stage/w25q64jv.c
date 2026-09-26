#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "tobudos.h"
#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

#define W25Q64JV_TIMEOUT            1000

static int w25q64jv_write_then_read(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    if (HAL_SPI_TransmitReceive(hspi, (uint8_t *)tx, rx, len, W25Q64JV_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

static int w25q64jv_write_only(struct w25q64jv_dev *dev, const uint8_t *tx, size_t len)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    if (HAL_SPI_TransmitReceive(hspi, (uint8_t *)tx, NULL, len, W25Q64JV_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = 0x00;

    uint8_t tx[1] = {W25Q64JV_CMD_JEDEC_ID};
    uint8_t rx[3] = {0};
    if (w25q64jv_write_then_read(dev, tx, rx, 4) != 0)
        return -1;
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t tx[4];
    tx[0] = W25Q64JV_CMD_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;

    uint8_t rx[4 + len];
    if (w25q64jv_write_then_read(dev, tx, rx, 4 + len) != 0)
        return -1;

    for (size_t i = 0; i < len; i++)
        buf[i] = rx[4 + i];
    return 0;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    uint8_t we_cmd[1] = {W25Q64JV_CMD_WRITE_ENABLE};
    if (w25q64jv_write_only(dev, we_cmd, 1) != 0)
        return -1;

    uint8_t tx[4 + len];
    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    for (size_t i = 0; i < len; i++)
        tx[4 + i] = buf[i];

    if (w25q64jv_write_only(dev, tx, 4 + len) != 0)
        return -1;
    return 0;
}
