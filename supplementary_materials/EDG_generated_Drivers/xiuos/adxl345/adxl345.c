#include "adxl345.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "bus.h"

#define ADXL345_SPI_CS 0

static int spi_write(struct adxl345_dev *dev, const uint8_t *data, size_t len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .rx_buf = 0,
        .len = len,
        .speed_hz = 5000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
    };
    if (ioctl(bus->fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        return -EIO;
    return 0;
}

static int spi_write_then_read(struct adxl345_dev *dev, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    uint8_t tx_buf[tx_len + rx_len];
    uint8_t rx_buf[tx_len + rx_len];
    memcpy(tx_buf, tx, tx_len);
    memset(tx_buf + tx_len, 0, rx_len);
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = tx_len + rx_len,
        .speed_hz = 5000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
    };
    if (ioctl(bus->fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        return -EIO;
    memcpy(rx, rx_buf + tx_len, rx_len);
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t cmd1[] = {0x2D, 0x00};
    if (spi_write(dev, cmd1, sizeof(cmd1)) < 0)
        return -EIO;
    uint8_t cmd2[] = {0x2D, 0x08};
    if (spi_write(dev, cmd2, sizeof(cmd2)) < 0)
        return -EIO;
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t cmd = 0xF2;
    uint8_t rx[6];
    if (spi_write_then_read(dev, &cmd, 1, rx, 6) < 0)
        return -EIO;
    *ax = (int16_t)(rx[0] | (rx[1] << 8));
    *ay = (int16_t)(rx[2] | (rx[3] << 8));
    *az = (int16_t)(rx[4] | (rx[5] << 8));
    return 0;
}