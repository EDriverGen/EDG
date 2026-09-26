#include "adxl345.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "bus.h"
#include "bus_pin.h"

#define ADXL345_SPI_DEVICE "/dev/spi0"

static int spi_write(struct adxl345_dev *dev, const uint8_t *data, size_t len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    if (!bus) return -EIO;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .rx_buf = 0,
        .len = len,
        .speed_hz = 5000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
    };
    int ret = ioctl(bus->fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0) return -EIO;
    return 0;
}

static int spi_write_then_read(struct adxl345_dev *dev, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    if (!bus) return -EIO;
    uint8_t buf[tx_len + rx_len];
    memcpy(buf, tx, tx_len);
    memset(buf + tx_len, 0, rx_len);
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)buf,
        .rx_buf = (unsigned long)buf,
        .len = tx_len + rx_len,
        .speed_hz = 5000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
    };
    int ret = ioctl(bus->fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0) return -EIO;
    memcpy(rx, buf + tx_len, rx_len);
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t cmd[2];
    cmd[0] = 0x2D;
    cmd[1] = 0x00;
    int ret = spi_write(dev, cmd, 2);
    if (ret) return ret;
    cmd[1] = 0x08;
    ret = spi_write(dev, cmd, 2);
    if (ret) return ret;
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t tx = 0xF2;
    uint8_t rx[6];
    int ret = spi_write_then_read(dev, &tx, 1, rx, 6);
    if (ret) return ret;
    *ax = (int16_t)(rx[0] | (rx[1] << 8));
    *ay = (int16_t)(rx[2] | (rx[3] << 8));
    *az = (int16_t)(rx[4] | (rx[5] << 8));
    return 0;
}