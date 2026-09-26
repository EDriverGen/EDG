#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>
#include "spi_if.h"

struct adxl345_dev {
    DevHandle spi_handle;
};

int adxl345_init(struct adxl345_dev *dev, DevHandle bus_handle);
int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az);

#endif