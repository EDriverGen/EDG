#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

struct adxl345_dev {
    void *bus_handle;
};

int adxl345_init(struct adxl345_dev *dev, void *bus_handle);
int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az);

#endif