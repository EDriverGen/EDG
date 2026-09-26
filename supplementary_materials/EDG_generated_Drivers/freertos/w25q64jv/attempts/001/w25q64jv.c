#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#include "freertos.h"
#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

#define W25Q64JV_PAGE_SIZE          256
#define W25Q64JV_TIMEOUT            1000

static int w25q64jv_transfer(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, uint16_t size)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    HAL_GPIO_WritePin(GPIOA, dev->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, (uint8_t *)tx, rx, size, W25Q64JV_TIMEOUT);
    HAL_GPIO_WritePin(GPIOA, dev->cs_pin, GPIO_PIN_SET);
    return (ret == HAL_OK) ? 0 : -1;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->cs_pin = GPIO_PIN_4;

    uint8_t tx[4] = {W25Q64JV_CMD_JEDEC_ID, 0, 0, 0};
    uint8_t rx[4] = {0};
    int ret = w25q64jv_transfer(dev, tx, rx, 4);
    if (ret != 0) return -1;
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t tx[4];
    tx[0] = W25Q64JV_CMD_READ_DATA;
    tx[1] = (uint8_t)(addr >> 16);
    tx[2] = (uint8_t)(addr >> 8);
    tx[3] = (uint8_t)(addr);

    uint8_t rx[4];
    int ret = w25q64jv_transfer(dev, tx, rx, 4);
    if (ret != 0) return -1;

    uint8_t *data_tx = (uint8_t *)calloc(len, 1);
    if (!data_tx) return -1;
    uint8_t *data_rx = (uint8_t *)malloc(len);
    if (!data_rx) { free(data_tx); return -1; }

    ret = w25q64jv_transfer(dev, data_tx, data_rx, len);
    if (ret == 0) {
        memcpy(buf, data_rx, len);
    }
    free(data_tx);
    free(data_rx);
    return ret;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0 || len > W25Q64JV_PAGE_SIZE) return -1;

    uint8_t tx_enable[1] = {W25Q64JV_CMD_WRITE_ENABLE};
    uint8_t rx_enable[1];
    int ret = w25q64jv_transfer(dev, tx_enable, rx_enable, 1);
    if (ret != 0) return -1;

    size_t total_len = 4 + len;
    uint8_t *tx = (uint8_t *)malloc(total_len);
    if (!tx) return -1;
    uint8_t *rx = (uint8_t *)calloc(total_len, 1);
    if (!rx) { free(tx); return -1; }

    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (uint8_t)(addr >> 16);
    tx[2] = (uint8_t)(addr >> 8);
    tx[3] = (uint8_t)(addr);
    memcpy(tx + 4, buf, len);

    ret = w25q64jv_transfer(dev, tx, rx, total_len);
    free(tx);
    free(rx);
    return ret;
}
