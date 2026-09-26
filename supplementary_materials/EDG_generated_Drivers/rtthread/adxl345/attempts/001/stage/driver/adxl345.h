#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

struct adxl345_device {
    struct rt_spi_device *spi;
};

int adxl345_init(struct adxl345_device *dev, struct rt_spi_device *spi);
int adxl345_read_accel_x(struct adxl345_device *dev, int32_t *ax);
int adxl345_read_accel_y(struct adxl345_device *dev, int32_t *ay);
int adxl345_read_accel_z(struct adxl345_device *dev, int32_t *az);

#endif /* ADXL345_H */