#include "adxl345_ref.h"

int adxl345_write_reg(struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2];
    if (dev == 0 || dev->spi == 0) {
        return -1;
    }
    tx[0] = (uint8_t)(reg & 0x3FU);
    tx[1] = val;
    return HAL_SPI_Transmit(dev->spi, tx, 2, 100) == HAL_OK ? 0 : -1;
}

int adxl345_read_reg(struct adxl345_device *dev, uint8_t reg, uint8_t *val)
{
    uint8_t tx[2];
    uint8_t rx[2];
    if (dev == 0 || dev->spi == 0 || val == 0) {
        return -1;
    }
    tx[0] = (uint8_t)(ADXL345_SPI_READ | (reg & 0x3FU));
    tx[1] = 0x00;
    if (HAL_SPI_TransmitReceive(dev->spi, tx, rx, 2, 100) != HAL_OK) {
        return -1;
    }
    *val = rx[1];
    return 0;
}

int adxl345_init(struct adxl345_device *dev, SPI_HandleTypeDef *spi, uint8_t range)
{
    uint8_t id = 0;
    if (dev == 0 || spi == 0) {
        return -1;
    }
    if (HAL_SPI_Init(spi) != HAL_OK) {
        return -1;
    }
    dev->spi = spi;
    if (adxl345_read_id(dev, &id) != 0 || id != ADXL345_DEVID) {
        return -1;
    }
    if (adxl345_write_reg(dev, ADXL345_REG_DATA_FMT,
                          (uint8_t)(ADXL345_FULL_RES_BIT | (range & 0x03U))) != 0) {
        return -1;
    }
    if (adxl345_write_reg(dev, ADXL345_REG_BW_RATE, 0x0A) != 0) {
        return -1;
    }
    if (adxl345_write_reg(dev, ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT) != 0) {
        return -1;
    }
    return 0;
}

int adxl345_read_id(struct adxl345_device *dev, uint8_t *id)
{
    return adxl345_read_reg(dev, ADXL345_REG_DEVID, id);
}

int adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel)
{
    uint8_t tx[7] = {0};
    uint8_t rx[7] = {0};
    if (dev == 0 || dev->spi == 0 || accel == 0) {
        return -1;
    }
    tx[0] = (uint8_t)(ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0);
    if (HAL_SPI_TransmitReceive(dev->spi, tx, rx, 7, 100) != HAL_OK) {
        return -1;
    }
    accel->x = (int16_t)((uint16_t)rx[1] | ((uint16_t)rx[2] << 8));
    accel->y = (int16_t)((uint16_t)rx[3] | ((uint16_t)rx[4] << 8));
    accel->z = (int16_t)((uint16_t)rx[5] | ((uint16_t)rx[6] << 8));
    return 0;
}

int adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x_mg, int32_t *y_mg, int32_t *z_mg)
{
    struct adxl345_accel a;
    if (x_mg == 0 || y_mg == 0 || z_mg == 0) {
        return -1;
    }
    if (adxl345_read_accel(dev, &a) != 0) {
        return -1;
    }
    *x_mg = (int32_t)a.x * ADXL345_SCALE_MG;
    *y_mg = (int32_t)a.y * ADXL345_SCALE_MG;
    *z_mg = (int32_t)a.z * ADXL345_SCALE_MG;
    return 0;
}
