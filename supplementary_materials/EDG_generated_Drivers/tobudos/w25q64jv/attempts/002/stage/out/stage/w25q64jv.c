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

static int w25q64jv_transfer(struct w25q64jv_dev *dev, uint8_t *tx, uint8_t *rx, uint16_t size)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    if (HAL_SPI_TransmitReceive(hspi, tx, rx, size, W25Q64JV_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = 0x00;

    uint8_t tx[4] = {W25Q64JV_CMD_JEDEC_ID, 0, 0, 0};
    uint8_t rx[4] = {0};
    if (w25q64jv_transfer(dev, tx, rx, 4) != 0)
        return -1;
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t tx[4 + 256];
    uint8_t rx[4 + 256];
    size_t total = 4 + len;
    if (total > sizeof(tx)) return -1;

    tx[0] = W25Q64JV_CMD_READ_DATA;
    tx[1] = (uint8_t)(addr >> 16);
    tx[2] = (uint8_t)(addr >> 8);
    tx[3] = (uint8_t)(addr);
    for (size_t i = 4; i < total; i++) tx[i] = 0;

    if (w25q64jv_transfer(dev, tx, rx, total) != 0)
        return -1;

    for (size_t i = 0; i < len; i++)
        buf[i] = rx[4 + i];
    return 0;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0 || len > 256) return -1;

    uint8_t tx[1] = {W25Q64JV_CMD_WRITE_ENABLE};
    uint8_t rx[1] = {0};
    if (w25q64jv_transfer(dev, tx, rx, 1) != 0)
        return -1;

    uint8_t tx2[4 + 256];
    uint8_t rx2[4 + 256];
    size_t total = 4 + len;
    tx2[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx2[1] = (uint8_t)(addr >> 16);
    tx2[2] = (uint8_t)(addr >> 8);
    tx2[3] = (uint8_t)(addr);
    for (size_t i = 0; i < len; i++)
        tx2[4 + i] = buf[i];

    if (w25q64jv_transfer(dev, tx2, rx2, total) != 0)
        return -1;

    return 0;
}
