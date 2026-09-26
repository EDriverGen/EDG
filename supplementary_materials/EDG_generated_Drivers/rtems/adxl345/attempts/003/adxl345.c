#include "adxl345.h"
#include <errno.h>
#include <string.h>
#include <stdint.h>

#define ADXL345_READ_CMD(reg) (0x80 | (reg))
#define ADXL345_MB_CMD(reg) (0xC0 | (reg))

static int spi_write(struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg, val };
    int ret = spi_write_then_read(dev->bus, tx, 2, NULL, 0);
    return (ret == 0) ? 0 : -EIO;
}

static int spi_read_multi(struct adxl345_device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    uint8_t cmd = ADXL345_MB_CMD(reg);
    uint8_t tx[1] = { cmd };
    uint8_t rx[1 + len];
    memset(rx, 0, sizeof(rx));
    int ret = spi_write_then_read(dev->bus, tx, 1, rx, 1 + len);
    if (ret != 0) return -EIO;
    memcpy(buf, rx + 1, len);
    return 0;
}

int adxl345_init(struct adxl345_device *dev, spi_bus bus_handle)
{
    dev->bus = bus_handle;
    int ret;
    ret = spi_write(dev, 0x2D, 0x00);
    if (ret != 0) return ret;
    ret = spi_write(dev, 0x2D, 0x08);
    if (ret != 0) return ret;
    return 0;
}

int adxl345_read_xyz(struct adxl345_device *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    int ret = spi_read_multi(dev, 0x32, buf, 6);
    if (ret != 0) return ret;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}