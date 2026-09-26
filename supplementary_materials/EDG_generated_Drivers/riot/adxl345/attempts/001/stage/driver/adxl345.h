#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

#include "riot.h"
typedef struct {
    spi_t bus;
    spi_cs_t cs;
} adxl345_t;

int adxl345_init(adxl345_t *dev, spi_t bus, spi_cs_t cs);
int adxl345_read_xyz(adxl345_t *dev, int16_t *ax, int16_t *ay, int16_t *az);

#endif /* ADXL345_H */
