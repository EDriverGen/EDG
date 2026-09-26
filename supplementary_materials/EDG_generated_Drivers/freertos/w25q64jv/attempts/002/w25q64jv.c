#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos.h"
#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

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
    dev->cs_pin = GPIO_PIN_4;

    uint8_t tx[1] = {W25Q64JV_CMD_JEDEC_ID};
    uint8_t rx[4] = {0};
    if (w25q64jv_transfer(dev, tx, rx, 1) != 0)
        return -1;
    if (w25q64jv_transfer(dev, NULL, rx, 3) != 0)
        return -1;
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    if (w25q64jv_transfer(dev, cmd, NULL, 4) != 0)
        return -1;
    if (w25q64jv_transfer(dev, NULL, buf, (uint16_t)len) != 0)
        return -1;
    return 0;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0 || len > 256) return -1;
    uint8_t we_cmd[1] = {W25Q64JV_CMD_WRITE_ENABLE};
    if (w25q64jv_transfer(dev, we_cmd, NULL, 1) != 0)
        return -1;
    uint8_t pp_cmd[4];
    pp_cmd[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    pp_cmd[1] = (uint8_t)(addr >> 16);
    pp_cmd[2] = (uint8_t)(addr >> 8);
    pp_cmd[3] = (uint8_t)(addr);
    if (w25q64jv_transfer(dev, pp_cmd, NULL, 4) != 0)
        return -1;
    if (w25q64jv_transfer(dev, buf, NULL, (uint16_t)len) != 0)
        return -1;
    return 0;
}
