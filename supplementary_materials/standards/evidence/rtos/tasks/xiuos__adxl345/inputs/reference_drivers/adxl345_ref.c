/*
 * ADXL345 accelerometer driver for XiUOS
 */
#include "adxl345_ref.h"

static int adxl345_xfer(struct adxl345_device *dev, const uint8_t *tx, uint8_t *rx, int len)
{
    struct SpiDataParam xfer;
    xfer.tx_buff = (void *)tx;
    xfer.rx_buff = rx;
    xfer.length = len;
    return PrivIoctl(dev->spi_fd, SPI_IOC_TRANSFER, &xfer);
}

int adxl345_init(struct adxl345_device *dev, const char *spi_path, uint8_t range)
{
    uint8_t tx[2], rx[2], id;
    if (!dev || !spi_path) return -1;
    dev->spi_fd = PrivOpen(spi_path, O_RDWR);
    if (dev->spi_fd < 0) return -1;

    tx[0] = ADXL345_SPI_READ | ADXL345_REG_DEVID; tx[1] = 0;
    if (adxl345_xfer(dev, tx, rx, 2)) return -1;
    id = rx[1];
    if (id != ADXL345_DEVID) { PrivClose(dev->spi_fd); dev->spi_fd = -1; return -1; }

    tx[0] = ADXL345_REG_DATA_FMT; tx[1] = ADXL345_FULL_RES_BIT | (range & 0x03);
    adxl345_xfer(dev, tx, rx, 2);
    tx[0] = ADXL345_REG_BW_RATE; tx[1] = 0x0A;
    adxl345_xfer(dev, tx, rx, 2);
    tx[0] = ADXL345_REG_POWER_CTL; tx[1] = ADXL345_MEASURE_BIT;
    adxl345_xfer(dev, tx, rx, 2);
    return 0;
}

void adxl345_deinit(struct adxl345_device *dev)
{ if (dev && dev->spi_fd >= 0) { PrivClose(dev->spi_fd); dev->spi_fd = -1; } }

int adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel)
{
    uint8_t tx[7] = {0}, rx[7] = {0};
    if (!dev || dev->spi_fd < 0 || !accel) return -1;
    tx[0] = ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0;
    if (adxl345_xfer(dev, tx, rx, 7)) return -1;
    accel->x = (int16_t)(rx[1] | ((uint16_t)rx[2] << 8));
    accel->y = (int16_t)(rx[3] | ((uint16_t)rx[4] << 8));
    accel->z = (int16_t)(rx[5] | ((uint16_t)rx[6] << 8));
    return 0;
}

int adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x, int32_t *y, int32_t *z)
{
    struct adxl345_accel a;
    if (adxl345_read_accel(dev, &a)) return -1;
    *x = (int32_t)a.x * ADXL345_SCALE_MG; *y = (int32_t)a.y * ADXL345_SCALE_MG;
    *z = (int32_t)a.z * ADXL345_SCALE_MG;
    return 0;
}
