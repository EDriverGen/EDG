#include "adxl345_ref.h"

#include <dev/spi/spi.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int adxl345_spi_xfer(struct adxl345_device *dev, const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    struct spi_ioc_transfer msg;
    int fd;
    if (dev == 0 || dev->spi_path == 0 || len == 0) {
        return -1;
    }
    fd = open(dev->spi_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    msg.tx_buf = (uint64_t)(uintptr_t)tx;
    msg.rx_buf = (uint64_t)(uintptr_t)rx;
    msg.len = len;
    msg.speed_hz = ADXL345_SPI_MAX_HZ;
    msg.delay_usecs = 0;
    msg.bits_per_word = 8;
    msg.cs_change = 0;
    msg.mode = SPI_MODE_3;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &msg) != 0) {
        (void)close(fd);
        return -1;
    }
    (void)close(fd);
    return 0;
}

int adxl345_write_reg(struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2];
    tx[0] = (uint8_t)(reg & 0x3FU);
    tx[1] = val;
    return adxl345_spi_xfer(dev, tx, 0, 2);
}

int adxl345_read_reg(struct adxl345_device *dev, uint8_t reg, uint8_t *val)
{
    uint8_t tx[2];
    uint8_t rx[2];
    if (val == 0) {
        return -1;
    }
    tx[0] = (uint8_t)(ADXL345_SPI_READ | (reg & 0x3FU));
    tx[1] = 0x00;
    if (adxl345_spi_xfer(dev, tx, rx, 2) != 0) {
        return -1;
    }
    *val = rx[1];
    return 0;
}

int adxl345_init(struct adxl345_device *dev, const char *spi_path, uint8_t range)
{
    int fd;
    uint8_t mode = SPI_MODE_3;
    uint8_t bits = 8;
    uint32_t speed = ADXL345_SPI_MAX_HZ;
    uint8_t id = 0;

    if (dev == 0 || spi_path == 0) {
        return -1;
    }
    fd = open(spi_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    (void)ioctl(fd, SPI_IOC_WR_MODE, &mode);
    (void)ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    (void)ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    (void)close(fd);
    dev->spi_path = spi_path;

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
    if (accel == 0) {
        return -1;
    }
    tx[0] = (uint8_t)(ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0);
    if (adxl345_spi_xfer(dev, tx, rx, 7) != 0) {
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
