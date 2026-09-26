#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_READ_STATUS_1  0x05
#define W25Q64JV_CMD_JEDEC_ID       0x9F

#define W25Q64JV_TIMEOUT            1000
#define W25Q64JV_PAGE_SIZE          256

static int w25q64jv_transfer(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, uint16_t size)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, (uint8_t *)tx, rx, size, W25Q64JV_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int w25q64jv_write_enable(struct w25q64jv_dev *dev)
{
    uint8_t tx[1] = {W25Q64JV_CMD_WRITE_ENABLE};
    uint8_t rx[1];
    return w25q64jv_transfer(dev, tx, rx, 1);
}

static int w25q64jv_wait_busy(struct w25q64jv_dev *dev)
{
    uint8_t tx[2] = {W25Q64JV_CMD_READ_STATUS_1, 0};
    uint8_t rx[2];
    int retries = 10000;
    while (retries--) {
        if (w25q64jv_transfer(dev, tx, rx, 2) != 0)
            return -1;
        if (!(rx[1] & 0x01))
            return 0;
        HAL_Delay(1);
    }
    return -1;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->cs_pin = GPIO_PIN_4;

    uint8_t tx[1] = {W25Q64JV_CMD_JEDEC_ID};
    uint8_t rx[4];
    if (w25q64jv_transfer(dev, tx, rx, 1) != 0)
        return -1;
    memset(rx, 0, sizeof(rx));
    if (w25q64jv_transfer(dev, tx, rx, 3) != 0)
        return -1;
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0 || len > W25Q64JV_PAGE_SIZE)
        return -1;

    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    uint8_t rx_cmd[4];
    if (w25q64jv_transfer(dev, cmd, rx_cmd, 4) != 0)
        return -1;

    uint8_t *dummy_tx = (uint8_t *)malloc(len);
    if (!dummy_tx)
        return -1;
    memset(dummy_tx, 0, len);
    int ret = w25q64jv_transfer(dev, dummy_tx, buf, (uint16_t)len);
    free(dummy_tx);
    return ret;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0 || len > W25Q64JV_PAGE_SIZE)
        return -1;

    if (w25q64jv_write_enable(dev) != 0)
        return -1;

    size_t total_len = 4 + len;
    uint8_t *tx = (uint8_t *)malloc(total_len);
    if (!tx)
        return -1;
    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (uint8_t)(addr >> 16);
    tx[2] = (uint8_t)(addr >> 8);
    tx[3] = (uint8_t)(addr);
    memcpy(tx + 4, buf, len);

    uint8_t *rx = (uint8_t *)malloc(total_len);
    if (!rx) {
        free(tx);
        return -1;
    }
    int ret = w25q64jv_transfer(dev, tx, rx, (uint16_t)total_len);
    free(tx);
    free(rx);
    if (ret != 0)
        return ret;

    return w25q64jv_wait_busy(dev);
}
