#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

struct spi_dev_s;

struct adxl345_dev_s {
    struct spi_dev_s *spi;
};

int adxl345_init(struct adxl345_dev_s *dev, struct spi_dev_s *bus_handle);
int adxl345_read_xyz(struct adxl345_dev_s *dev, int16_t *ax, int16_t *ay, int16_t *az);

#endif /* ADXL345_H */