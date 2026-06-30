/*
 * ADXL345 accelerometer driver for NuttX (SPI)
 */
#include "adxl345_ref.h"
#include <errno.h>

int adxl345_write_reg(FAR struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    if (!dev || !dev->spi) return -EINVAL;
    SPI_LOCK(dev->spi, true);
    SPI_SETFREQUENCY(dev->spi, ADXL345_SPI_MAX_HZ);
    SPI_SETMODE(dev->spi, SPIDEV_MODE3);
    SPI_SETBITS(dev->spi, 8);
    SPI_SELECT(dev->spi, dev->devid, true);
    SPI_SEND(dev->spi, reg & 0x3F);
    SPI_SEND(dev->spi, val);
    SPI_SELECT(dev->spi, dev->devid, false);
    SPI_LOCK(dev->spi, false);
    return 0;
}

int adxl345_read_reg(FAR struct adxl345_device *dev, uint8_t reg, FAR uint8_t *val)
{
    if (!dev || !dev->spi || !val) return -EINVAL;
    SPI_LOCK(dev->spi, true);
    SPI_SETFREQUENCY(dev->spi, ADXL345_SPI_MAX_HZ);
    SPI_SETMODE(dev->spi, SPIDEV_MODE3);
    SPI_SETBITS(dev->spi, 8);
    SPI_SELECT(dev->spi, dev->devid, true);
    SPI_SEND(dev->spi, ADXL345_SPI_READ | (reg & 0x3F));
    *val = (uint8_t)SPI_SEND(dev->spi, 0xFF);
    SPI_SELECT(dev->spi, dev->devid, false);
    SPI_LOCK(dev->spi, false);
    return 0;
}

int adxl345_init(FAR struct adxl345_device *dev, FAR struct spi_dev_s *spi, uint32_t devid, uint8_t range)
{
    uint8_t id;
    if (!dev || !spi) return -EINVAL;
    dev->spi = spi; dev->devid = devid;
    if (adxl345_read_reg(dev, ADXL345_REG_DEVID, &id) || id != ADXL345_DEVID) return -EIO;
    adxl345_write_reg(dev, ADXL345_REG_DATA_FMT, ADXL345_FULL_RES_BIT | (range & 0x03));
    adxl345_write_reg(dev, ADXL345_REG_BW_RATE, 0x0A);
    adxl345_write_reg(dev, ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT);
    return 0;
}

int adxl345_read_accel(FAR struct adxl345_device *dev, FAR struct adxl345_accel *accel)
{
    uint8_t buf[6];
    if (!dev || !dev->spi || !accel) return -EINVAL;
    SPI_LOCK(dev->spi, true);
    SPI_SELECT(dev->spi, dev->devid, true);
    SPI_SEND(dev->spi, ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0);
    SPI_RECVBLOCK(dev->spi, buf, 6);
    SPI_SELECT(dev->spi, dev->devid, false);
    SPI_LOCK(dev->spi, false);
    accel->x = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    accel->y = (int16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    accel->z = (int16_t)(buf[4] | ((uint16_t)buf[5] << 8));
    return 0;
}

int adxl345_read_accel_mg(FAR struct adxl345_device *dev, FAR int32_t *x, FAR int32_t *y, FAR int32_t *z)
{
    struct adxl345_accel a;
    int ret = adxl345_read_accel(dev, &a);
    if (ret) return ret;
    *x = (int32_t)a.x * ADXL345_SCALE_MG; *y = (int32_t)a.y * ADXL345_SCALE_MG; *z = (int32_t)a.z * ADXL345_SCALE_MG;
    return 0;
}
