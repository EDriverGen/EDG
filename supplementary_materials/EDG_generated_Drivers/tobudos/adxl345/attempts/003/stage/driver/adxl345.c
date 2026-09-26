#include "adxl345.h"
#include <stddef.h>

#include "tobudos.h"
#include "stm32f1xx_hal.h"
#define ADXL345_READ_CMD(reg) (0x80 | (reg))
#define ADXL345_MB_CMD(reg) (0xC0 | (reg))

static int adxl345_write_reg(struct adxl345_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg, val };
    uint8_t rx[2] = {0};
    if (HAL_SPI_TransmitReceive((SPI_HandleTypeDef *)dev->bus_handle, tx, rx, 2, 100) != HAL_OK)
        return -1;
    return 0;
}

static int adxl345_read_burst(struct adxl345_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t cmd = ADXL345_MB_CMD(reg);
    uint8_t tx[7] = {0};
    uint8_t rx[7] = {0};
    if (len > 6) len = 6;
    tx[0] = cmd;
    if (HAL_SPI_TransmitReceive((SPI_HandleTypeDef *)dev->bus_handle, tx, rx, len + 1, 100) != HAL_OK)
        return -1;
    for (uint16_t i = 0; i < len; i++)
        buf[i] = rx[i + 1];
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    if (adxl345_write_reg(dev, 0x2D, 0x00) != 0)
        return -1;
    if (adxl345_write_reg(dev, 0x2D, 0x08) != 0)
        return -1;
    HAL_Delay(12);
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    if (adxl345_read_burst(dev, 0x32, buf, 6) != 0)
        return -1;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}
