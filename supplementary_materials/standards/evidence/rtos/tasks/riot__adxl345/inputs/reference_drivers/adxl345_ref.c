/*
 * ADXL345 accelerometer driver for RIOT (SPI)
 */
#include "adxl345_ref.h"

int adxl345_write_reg(struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg & 0x3F, val };
    if (!dev) return -1;
    spi_acquire(dev->bus, dev->cs, SPI_MODE_3, SPI_CLK_5MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, tx, NULL, 2);
    spi_release(dev->bus);
    return 0;
}

int adxl345_read_reg(struct adxl345_device *dev, uint8_t reg, uint8_t *val)
{
    uint8_t tx[2] = { ADXL345_SPI_READ | (reg & 0x3F), 0 };
    uint8_t rx[2] = {0};
    if (!dev || !val) return -1;
    spi_acquire(dev->bus, dev->cs, SPI_MODE_3, SPI_CLK_5MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, tx, rx, 2);
    spi_release(dev->bus);
    *val = rx[1];
    return 0;
}

int adxl345_init(struct adxl345_device *dev, spi_t bus, spi_cs_t cs, uint8_t range)
{
    uint8_t id;
    if (!dev) return -1;
    dev->bus = bus; dev->cs = cs;
    spi_init(bus); spi_init_cs(bus, cs);
    if (adxl345_read_reg(dev, ADXL345_REG_DEVID, &id) || id != ADXL345_DEVID) return -1;
    adxl345_write_reg(dev, ADXL345_REG_DATA_FMT, ADXL345_FULL_RES_BIT | (range & 0x03));
    adxl345_write_reg(dev, ADXL345_REG_BW_RATE, 0x0A);
    adxl345_write_reg(dev, ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT);
    return 0;
}

int adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel)
{
    uint8_t tx[7] = {0}, rx[7] = {0};
    if (!dev || !accel) return -1;
    tx[0] = ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0;
    spi_acquire(dev->bus, dev->cs, SPI_MODE_3, SPI_CLK_5MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, tx, rx, 7);
    spi_release(dev->bus);
    accel->x = (int16_t)(rx[1] | ((uint16_t)rx[2] << 8));
    accel->y = (int16_t)(rx[3] | ((uint16_t)rx[4] << 8));
    accel->z = (int16_t)(rx[5] | ((uint16_t)rx[6] << 8));
    return 0;
}

int adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x, int32_t *y, int32_t *z)
{
    struct adxl345_accel a;
    if (adxl345_read_accel(dev, &a)) return -1;
    *x = (int32_t)a.x * ADXL345_SCALE_MG; *y = (int32_t)a.y * ADXL345_SCALE_MG; *z = (int32_t)a.z * ADXL345_SCALE_MG;
    return 0;
}
