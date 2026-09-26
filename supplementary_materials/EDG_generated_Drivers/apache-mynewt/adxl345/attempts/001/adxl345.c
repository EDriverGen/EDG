#include "adxl345.h"
#include <assert.h>
#include <stddef.h>

#include <os/os_time.h>
#define ADXL345_SPI_CS 0

static int adxl345_spi_write(struct adxl345_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg, val };
    int rc = hal_spi_txrx(ADXL345_SPI_CS, tx, NULL, 2);
    if (rc != 0) return -1;
    return 0;
}

static int adxl345_spi_read(struct adxl345_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    uint8_t tx[32];
    uint8_t rx[32];
    int i;
    if (len > 31) return -1;
    tx[0] = reg;
    for (i = 1; i <= len; i++) tx[i] = 0;
    int rc = hal_spi_txrx(ADXL345_SPI_CS, tx, rx, len + 1);
    if (rc != 0) return -1;
    for (i = 0; i < len; i++) buf[i] = rx[i + 1];
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    (void)bus_handle;
    int rc;
    rc = adxl345_spi_write(dev, 0x2D, 0x00);
    if (rc != 0) return -1;
    rc = adxl345_spi_write(dev, 0x2D, 0x08);
    if (rc != 0) return -1;
    os_time_delay(12);
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    int rc = adxl345_spi_read(dev, 0xF2, buf, 6);
    if (rc != 0) return -1;
    int16_t raw_x = (int16_t)(buf[0] | (buf[1] << 8));
    int16_t raw_y = (int16_t)(buf[2] | (buf[3] << 8));
    int16_t raw_z = (int16_t)(buf[4] | (buf[5] << 8));
    *x = raw_x;
    *y = raw_y;
    *z = raw_z;
    return 0;
}
