#include "adxl345.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "arch.h"

#include <nuttx/spi/spi.h>
#include "nuttx.h"
#define ADXL345_DEVID       0x00
#define ADXL345_POWER_CTL   0x2D
#define ADXL345_DATAX0      0x32

#define ADXL345_READ_BIT    0x80
#define ADXL345_MB_BIT      0x40

static int adxl345_write_reg(struct adxl345_dev_s *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2];
    tx[0] = reg;
    tx[1] = val;
    SPI_SELECT(dev->spi, 0, true);
    int ret = SPI_SEND(dev->spi, tx[0]);
    if (ret < 0) {
        SPI_SELECT(dev->spi, 0, false);
        return -EIO;
    }
    ret = SPI_SEND(dev->spi, tx[1]);
    SPI_SELECT(dev->spi, 0, false);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int adxl345_read_burst(struct adxl345_dev_s *dev, uint8_t reg, uint8_t *buf, int len)
{
    uint8_t cmd = reg | ADXL345_READ_BIT | ADXL345_MB_BIT;
    SPI_SELECT(dev->spi, 0, true);
    int ret = SPI_SEND(dev->spi, cmd);
    if (ret < 0) {
        SPI_SELECT(dev->spi, 0, false);
        return -EIO;
    }
    for (int i = 0; i < len; i++) {
        ret = SPI_SEND(dev->spi, 0xFF);
        if (ret < 0) {
            SPI_SELECT(dev->spi, 0, false);
            return -EIO;
        }
        buf[i] = (uint8_t)ret;
    }
    SPI_SELECT(dev->spi, 0, false);
    return 0;
}

int adxl345_init(struct adxl345_dev_s *dev, struct spi_dev_s *bus_handle)
{
    dev->spi = bus_handle;
    int ret;
    ret = adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x00);
    if (ret < 0) return ret;
    ret = adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x08);
    if (ret < 0) return ret;
    up_mdelay(12);
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev_s *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    int ret = adxl345_read_burst(dev, ADXL345_DATAX0, buf, 6);
    if (ret < 0) return ret;
    *ax = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    *ay = (int16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    *az = (int16_t)(buf[4] | ((uint16_t)buf[5] << 8));
    return 0;
}
