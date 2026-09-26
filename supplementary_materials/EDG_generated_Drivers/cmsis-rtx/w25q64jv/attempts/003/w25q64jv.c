#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "cmsis_rtx.h"
#define W25Q64JV_CMD_READ_DATA  0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID   0x9F
#define W25Q64JV_CMD_READ_STATUS1 0x05

#define W25Q64JV_TIMEOUT 1000

static int w25q64jv_transfer(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, uint16_t size)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    if (HAL_SPI_TransmitReceive(hspi, (uint8_t *)tx, rx, size, W25Q64JV_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

static int w25q64jv_write_cmd_addr(struct w25q64jv_dev *dev, uint8_t cmd, uint32_t addr)
{
    uint8_t tx[4] = { cmd, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
    uint8_t rx[4];
    return w25q64jv_transfer(dev, tx, rx, 4);
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t tx[1] = { W25Q64JV_CMD_JEDEC_ID };
    uint8_t rx[4] = {0};
    int ret = w25q64jv_transfer(dev, tx, rx, 1);
    if (ret != 0) return ret;
    tx[0] = 0x00;
    ret = w25q64jv_transfer(dev, tx, rx, 3);
    if (ret != 0) return ret;
    HAL_Delay(1);
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t tx[4] = { W25Q64JV_CMD_READ_DATA, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
    uint8_t rx[4];
    int ret = w25q64jv_transfer(dev, tx, rx, 4);
    if (ret != 0) return ret;
    uint8_t *dummy_tx = (uint8_t *)buf;
    for (size_t i = 0; i < len; i++) dummy_tx[i] = 0;
    return w25q64jv_transfer(dev, dummy_tx, buf, len);
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t tx_enable[1] = { W25Q64JV_CMD_WRITE_ENABLE };
    uint8_t rx_enable[1];
    int ret = w25q64jv_transfer(dev, tx_enable, rx_enable, 1);
    if (ret != 0) return ret;
    uint8_t tx_cmd[4] = { W25Q64JV_CMD_PAGE_PROGRAM, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
    uint8_t rx_cmd[4];
    ret = w25q64jv_transfer(dev, tx_cmd, rx_cmd, 4);
    if (ret != 0) return ret;
    uint8_t *tx_data = (uint8_t *)buf;
    uint8_t *rx_data = (uint8_t *)buf;
    return w25q64jv_transfer(dev, tx_data, rx_data, len);
}
