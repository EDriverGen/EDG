#include "adxl345.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "bus.h"
#include "bus_pin.h"

#define ADXL345_SPI_READ 0x80
#define ADXL345_SPI_MB 0x40

static int spi_write(struct adxl345_dev *dev, uint8_t reg, uint8_t data) {
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    uint8_t tx[2] = { reg & 0x3F, data };
    uint8_t rx[2];
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 2,
        .speed_hz = 5000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0
    };
    if (ioctl(bus->private_data, SPI_IOC_MESSAGE(1), &tr) < 0)
        return -EIO;
    return 0;
}

static int spi_write_then_read(struct adxl345_dev *dev, uint8_t reg, uint8_t *buf, size_t len) {
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    uint8_t tx[1] = { reg };
    uint8_t rx[1 + len];
    struct spi_ioc_transfer tr[2];
    memset(tr, 0, sizeof(tr));
    tr[0].tx_buf = (unsigned long)tx;
    tr[0].rx_buf = (unsigned long)rx;
    tr[0].len = 1;
    tr[0].speed_hz = 5000000;
    tr[0].delay_usecs = 0;
    tr[0].bits_per_word = 8;
    tr[0].cs_change = 0;
    tr[1].tx_buf = (unsigned long)NULL;
    tr[1].rx_buf = (unsigned long)(rx + 1);
    tr[1].len = len;
    tr[1].speed_hz = 5000000;
    tr[1].delay_usecs = 0;
    tr[1].bits_per_word = 8;
    tr[1].cs_change = 0;
    if (ioctl(bus->private_data, SPI_IOC_MESSAGE(2), tr) < 0)
        return -EIO;
    memcpy(buf, rx + 1, len);
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    int ret;
    ret = spi_write(dev, 0x2D, 0x00);
    if (ret) return ret;
    ret = spi_write(dev, 0x2D, 0x08);
    if (ret) return ret;
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    int ret = spi_write_then_read(dev, 0xF2, buf, 6);
    if (ret) return ret;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}