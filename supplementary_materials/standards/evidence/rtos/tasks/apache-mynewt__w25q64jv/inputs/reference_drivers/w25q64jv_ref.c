/*
 * W25Q64JV SPI NOR Flash for Apache Mynewt + STM32 HAL.
 */
#include "w25q64jv_ref.h"

static void cs_low(struct w25q64jv_device *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}
static void cs_high(struct w25q64jv_device *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

/* ------------------------------------------------------------------ */
static int w25q64jv_wait_ready(struct w25q64jv_device *dev)
{
    uint8_t cmd = W25Q64_CMD_READ_SR1;
    uint8_t sr1;
    for (int i = 0; i < 10000; i++) {
        cs_low(dev);
        HAL_SPI_Transmit(dev->hspi, &cmd, 1, 100);
        HAL_SPI_Receive(dev->hspi, &sr1, 1, 100);
        cs_high(dev);
        if (!(sr1 & W25Q64_SR1_BUSY))
            return 0;
        {volatile int _x; for(_x=0;_x<100000;_x++);};
    }
    return -1;
}

/* ------------------------------------------------------------------ */
static int w25q64jv_write_enable(struct w25q64jv_device *dev)
{
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
    cs_low(dev);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(dev->hspi, &cmd, 1, 100);
    cs_high(dev);
    return (rc == HAL_OK) ? 0 : -1;
}

/* ------------------------------------------------------------------ */
int w25q64jv_init(struct w25q64jv_device *dev, SPI_HandleTypeDef *hspi,
                  GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    if (!dev || !hspi) return -1;
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;
    cs_high(dev);
    return 0;
}

/* ------------------------------------------------------------------ */
int w25q64jv_probe(struct w25q64jv_device *dev)
{
    uint8_t cmd = W25Q64_CMD_JEDEC_ID;
    uint8_t id[3];
    cs_low(dev);
    if (HAL_SPI_Transmit(dev->hspi, &cmd, 1, 100) != HAL_OK) {
        cs_high(dev); return -1;
    }
    if (HAL_SPI_Receive(dev->hspi, id, 3, 100) != HAL_OK) {
        cs_high(dev); return -1;
    }
    cs_high(dev);
    if (id[0] != W25Q64_JEDEC_MFR_WINBOND) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
int w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr,
                  uint8_t *buf, uint32_t len)
{
    if (!dev || !buf || len == 0) return -1;
    uint8_t cmd[4] = {
        W25Q64_CMD_READ_DATA,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr),
    };
    cs_low(dev);
    if (HAL_SPI_Transmit(dev->hspi, cmd, 4, 100) != HAL_OK) {
        cs_high(dev); return -1;
    }
    HAL_StatusTypeDef rc = HAL_SPI_Receive(dev->hspi, buf, len, 100);
    cs_high(dev);
    return (rc == HAL_OK) ? 0 : -1;
}

/* ------------------------------------------------------------------ */
int w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr,
                   const uint8_t *buf, uint32_t len)
{
    if (!dev || !buf || len == 0) return -1;
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t page_rem = W25Q64JV_PAGE_SIZE -
                            ((addr + offset) % W25Q64JV_PAGE_SIZE);
        uint32_t chunk = (len - offset < page_rem) ? (len - offset) : page_rem;
        w25q64jv_write_enable(dev);
        uint8_t hdr[4] = {
            W25Q64_CMD_PAGE_PROGRAM,
            (uint8_t)((addr + offset) >> 16),
            (uint8_t)((addr + offset) >> 8),
            (uint8_t)(addr + offset),
        };
        cs_low(dev);
        if (HAL_SPI_Transmit(dev->hspi, hdr, 4, 100) != HAL_OK) {
            cs_high(dev); return -1;
        }
        if (HAL_SPI_Transmit(dev->hspi, (uint8_t *)(buf + offset),
                             chunk, 100) != HAL_OK) {
            cs_high(dev); return -1;
        }
        cs_high(dev);
        if (w25q64jv_wait_ready(dev) != 0) return -1;
        offset += chunk;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
int w25q64jv_erase_sector(struct w25q64jv_device *dev, uint32_t addr)
{
    w25q64jv_write_enable(dev);
    uint8_t cmd[4] = {
        W25Q64_CMD_SECTOR_ERASE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr),
    };
    cs_low(dev);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(dev->hspi, cmd, 4, 100);
    cs_high(dev);
    if (rc != HAL_OK) return -1;
    return w25q64jv_wait_ready(dev);
}
