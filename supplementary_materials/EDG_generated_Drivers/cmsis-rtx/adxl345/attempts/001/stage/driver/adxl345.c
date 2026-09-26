#include "adxl345.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#include "stm32f1xx_hal.h"
#define ADXL345_READ_BIT 0x80
#define ADXL345_MB_BIT   0x40

#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATAX0    0x32

static int adxl345_write_reg(struct adxl345_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg & 0x3F, val };
    uint8_t rx[2] = {0};
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(dev->spi, tx, rx, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int adxl345_read_burst(struct adxl345_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t cmd = (reg & 0x3F) | ADXL345_READ_BIT | ADXL345_MB_BIT;
    uint8_t tx[32];
    uint8_t rx[32];
    if (len > 31) return -1;
    tx[0] = cmd;
    for (uint16_t i = 1; i <= len; i++) tx[i] = 0;
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(dev->spi, tx, rx, len + 1, 100);
    if (ret != HAL_OK) return -1;
    for (uint16_t i = 0; i < len; i++) buf[i] = rx[i + 1];
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -1;
    dev->spi = (SPI_HandleTypeDef *)bus_handle;
    if (adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x00) != 0) return -1;
    if (adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x08) != 0) return -1;
    HAL_Delay(12);
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    if (!dev || !ax || !ay || !az) return -1;
    uint8_t buf[6];
    if (adxl345_read_burst(dev, ADXL345_DATAX0, buf, 6) != 0) return -1;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}
