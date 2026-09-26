#include "w25q64jv.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "tobudos.h"
#define W25Q64JV_CMD_READ_DATA    0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID     0x9F

#define W25Q64JV_TIMEOUT 1000

static void cs_low(struct w25q64jv_dev *dev) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

static void cs_high(struct w25q64jv_dev *dev) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

static int spi_transfer(struct w25q64jv_dev *dev, uint8_t *tx, uint8_t *rx, uint16_t size) {
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    if (HAL_SPI_TransmitReceive(hspi, tx, rx, size, W25Q64JV_TIMEOUT) != HAL_OK) {
        return -1;
    }
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->cs_pin = GPIO_PIN_4;
    dev->cs_port = (uint8_t)(uintptr_t)GPIOA;

    HAL_Delay(5);

    uint8_t tx[4] = {W25Q64JV_CMD_JEDEC_ID, 0, 0, 0};
    uint8_t rx[4] = {0};
    cs_low(dev);
    int ret = spi_transfer(dev, tx, rx, 4);
    cs_high(dev);
    if (ret != 0) return ret;

    if (rx[1] != 0xEF || rx[2] != 0x40 || rx[3] != 0x17) {
        return -1;
    }
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len) {
    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    cs_low(dev);
    int ret = spi_transfer(dev, cmd, cmd, 4);
    if (ret != 0) {
        cs_high(dev);
        return ret;
    }
    ret = spi_transfer(dev, buf, buf, (uint16_t)len);
    cs_high(dev);
    return ret;
}

int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len) {
    if (len > 256) return -1;

    uint8_t tx[4 + 256];
    tx[0] = W25Q64JV_CMD_WRITE_ENABLE;
    cs_low(dev);
    int ret = spi_transfer(dev, tx, tx, 1);
    cs_high(dev);
    if (ret != 0) return ret;

    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (uint8_t)(addr >> 16);
    tx[2] = (uint8_t)(addr >> 8);
    tx[3] = (uint8_t)(addr);
    for (size_t i = 0; i < len; i++) {
        tx[4 + i] = buf[i];
    }
    cs_low(dev);
    ret = spi_transfer(dev, tx, tx, (uint16_t)(4 + len));
    cs_high(dev);
    if (ret != 0) return ret;

    HAL_Delay(3);
    return 0;
}
