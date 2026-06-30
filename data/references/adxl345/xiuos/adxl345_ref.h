/*
 * ADXL345 3-axis accelerometer driver for XiUOS (SPI)
 */
#ifndef ADXL345_REF_H
#define ADXL345_REF_H

#include <transform.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADXL345_SPI_MAX_HZ    5000000
#define ADXL345_DEVID         0xE5
#define ADXL345_REG_DEVID     0x00
#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_DATA_FMT  0x31
#define ADXL345_REG_BW_RATE   0x2C
#define ADXL345_REG_DATAX0    0x32
#define ADXL345_MEASURE_BIT   (1u << 3)
#define ADXL345_FULL_RES_BIT  (1u << 3)
#define ADXL345_SPI_READ      0x80
#define ADXL345_SPI_MB        0x40
#define ADXL345_RANGE_2G      0x00
#define ADXL345_RANGE_4G      0x01
#define ADXL345_RANGE_8G      0x02
#define ADXL345_RANGE_16G     0x03
#define ADXL345_SCALE_MG      4  /* ~3.9 mg/LSB, use 4 for integer math */

struct adxl345_accel { int16_t x; int16_t y; int16_t z; };

struct adxl345_device
{
    int spi_fd;
};

int adxl345_init(struct adxl345_device *dev, const char *spi_path, uint8_t range);
void adxl345_deinit(struct adxl345_device *dev);
int adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel);
int adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x, int32_t *y, int32_t *z);

#ifdef __cplusplus
}
#endif
#endif
