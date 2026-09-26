#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

#include <dev/spi/spi.h>
struct adxl345_device {
    spi_bus bus;
};

int adxl345_init(struct adxl345_device *dev, spi_bus bus_handle);
int adxl345_read_xyz(struct adxl345_device *dev, int16_t *ax, int16_t *ay, int16_t *az);

#endif
