#include "adxl345.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "threadx.h"
#define ADXL345_SPI_TIMEOUT 100

static int adxl345_spi_write(struct adxl345_dev *dev, uint8_t reg, uint8_t data)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    uint8_t tx[2] = {reg, data};
    uint8_t rx[2] = {0};
    if (HAL_SPI_TransmitReceive(hspi, tx, rx, 2, ADXL345_SPI_TIMEOUT) != HAL_OK)
        return -1;
    return 0;
}

static int adxl345_spi_read(struct adxl345_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    uint8_t tx[1 + len];
    uint8_t rx[1 + len];
    tx[0] = reg;
    for (uint16_t i = 1; i < 1 + len; i++)
        tx[i] = 0;
    if (HAL_SPI_TransmitReceive(hspi, tx, rx, 1 + len, ADXL345_SPI_TIMEOUT) != HAL_OK)
        return -1;
    for (uint16_t i = 0; i < len; i++)
        buf[i] = rx[i + 1];
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    if (adxl345_spi_write(dev, 0x2D, 0x00) != 0)
        return -1;
    if (adxl345_spi_write(dev, 0x2D, 0x08) != 0)
        return -1;
    HAL_Delay(12);
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    if (adxl345_spi_read(dev, 0xF2, buf, 6) != 0)
        return -1;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}
