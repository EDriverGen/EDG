#include "adxl345.h"
#include <errno.h>
#include <string.h>

#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_DATAX0    0x32
#define ADXL345_READ_CMD(reg) (0x80 | (reg))
#define ADXL345_MB_CMD(reg)   (0xC0 | (reg))

static int spi_write(struct adxl345_device *dev, uint8_t reg, uint8_t val) {
    uint8_t tx[2] = { reg, val };
    uint8_t rx[2];
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 2,
        .speed_hz = 5000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
        .mode = 0,
    };
    if (ioctl(dev->bus, 0x40206C00 | (sizeof(tr) << 16) | 0x01, &tr) < 0)
        return -EIO;
    return 0;
}

static int spi_write_then_read(struct adxl345_device *dev, uint8_t cmd, uint8_t *buf, size_t len) {
    uint8_t tx[1 + 6];
    uint8_t rx[1 + 6];
    size_t total = 1 + len;
    tx[0] = cmd;
    memset(&tx[1], 0, len);
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = total,
        .speed_hz = 5000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
        .mode = 0,
    };
    if (ioctl(dev->bus, 0x40206C00 | (sizeof(tr) << 16) | 0x01, &tr) < 0)
        return -EIO;
    memcpy(buf, &rx[1], len);
    return 0;
}

int adxl345_init(struct adxl345_device *dev, spi_bus bus_handle) {
    dev->bus = bus_handle;
    int ret;
    ret = spi_write(dev, ADXL345_REG_POWER_CTL, 0x00);
    if (ret) return ret;
    ret = spi_write(dev, ADXL345_REG_POWER_CTL, 0x08);
    if (ret) return ret;
    return 0;
}

int adxl345_read_xyz(struct adxl345_device *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    int ret = spi_write_then_read(dev, ADXL345_MB_CMD(ADXL345_REG_DATAX0), buf, 6);
    if (ret) return ret;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}