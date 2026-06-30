/*
 * ADXL345 accelerometer driver for OpenHarmony HDF
 */
#include "adxl345_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG adxl345

static int32_t adxl345_spi_xfer(DevHandle h, const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    struct SpiMsg msg = { .wbuf=(uint8_t*)tx, .rbuf=rx, .len=len, .speed=ADXL345_SPI_MAX_HZ, .keepCs=0 };
    return SpiTransfer(h, &msg, 1);
}

int32_t adxl345_init(struct adxl345_device *dev, uint32_t bus, uint32_t cs, uint8_t range)
{
    struct SpiDevInfo info = { .busNum = bus, .csNum = cs };
    uint8_t tx[2], rx[2], id;
    if (!dev) return HDF_ERR_INVALID_PARAM;
    dev->spi_handle = SpiOpen(&info);
    if (!dev->spi_handle) return HDF_FAILURE;

    tx[0] = ADXL345_SPI_READ | ADXL345_REG_DEVID; tx[1] = 0;
    if (adxl345_spi_xfer(dev->spi_handle, tx, rx, 2) != HDF_SUCCESS) return HDF_FAILURE;
    id = rx[1];
    if (id != ADXL345_DEVID) { SpiClose(dev->spi_handle); return HDF_FAILURE; }

    tx[0] = ADXL345_REG_DATA_FMT; tx[1] = ADXL345_FULL_RES_BIT | (range & 0x03);
    adxl345_spi_xfer(dev->spi_handle, tx, rx, 2);
    tx[0] = ADXL345_REG_BW_RATE; tx[1] = 0x0A;
    adxl345_spi_xfer(dev->spi_handle, tx, rx, 2);
    tx[0] = ADXL345_REG_POWER_CTL; tx[1] = ADXL345_MEASURE_BIT;
    adxl345_spi_xfer(dev->spi_handle, tx, rx, 2);
    return HDF_SUCCESS;
}

void adxl345_deinit(struct adxl345_device *dev)
{ if (dev && dev->spi_handle) { SpiClose(dev->spi_handle); dev->spi_handle = NULL; } }

int32_t adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel)
{
    uint8_t tx[7] = {0}, rx[7] = {0};
    if (!dev || !dev->spi_handle || !accel) return HDF_ERR_INVALID_PARAM;
    tx[0] = ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0;
    if (adxl345_spi_xfer(dev->spi_handle, tx, rx, 7) != HDF_SUCCESS) return HDF_FAILURE;
    accel->x = (int16_t)(rx[1] | ((uint16_t)rx[2] << 8));
    accel->y = (int16_t)(rx[3] | ((uint16_t)rx[4] << 8));
    accel->z = (int16_t)(rx[5] | ((uint16_t)rx[6] << 8));
    return HDF_SUCCESS;
}

int32_t adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x, int32_t *y, int32_t *z)
{
    struct adxl345_accel a;
    int32_t ret = adxl345_read_accel(dev, &a);
    if (ret != HDF_SUCCESS) return ret;
    *x = (int32_t)a.x * ADXL345_SCALE_MG; *y = (int32_t)a.y * ADXL345_SCALE_MG; *z = (int32_t)a.z * ADXL345_SCALE_MG;
    return HDF_SUCCESS;
}
