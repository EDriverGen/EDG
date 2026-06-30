#ifndef ADXL345_APACHE_MYNEWT_REF_H
#define ADXL345_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_spi.h"
#include <stdint.h>

#define ADXL345_SPI_MAX_HZ    5000000U
#define ADXL345_DEVID         0xE5U
#define ADXL345_REG_DEVID     0x00U
#define ADXL345_REG_BW_RATE   0x2CU
#define ADXL345_REG_POWER_CTL 0x2DU
#define ADXL345_REG_DATA_FMT  0x31U
#define ADXL345_REG_DATAX0    0x32U
#define ADXL345_MEASURE_BIT   (1U << 3)
#define ADXL345_FULL_RES_BIT  (1U << 3)
#define ADXL345_SPI_READ      0x80U
#define ADXL345_SPI_MB        0x40U
#define ADXL345_RANGE_2G      0x00U
#define ADXL345_RANGE_4G      0x01U
#define ADXL345_RANGE_8G      0x02U
#define ADXL345_RANGE_16G     0x03U
#define ADXL345_SCALE_MG      4

struct adxl345_accel {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct adxl345_device {
    int spi_num;
};

int adxl345_init(struct adxl345_device *dev, int spi_num, uint8_t range);
int adxl345_read_id(struct adxl345_device *dev, uint8_t *id);
int adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel);
int adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x_mg, int32_t *y_mg, int32_t *z_mg);
int adxl345_write_reg(struct adxl345_device *dev, uint8_t reg, uint8_t val);
int adxl345_read_reg(struct adxl345_device *dev, uint8_t reg, uint8_t *val);

#endif
