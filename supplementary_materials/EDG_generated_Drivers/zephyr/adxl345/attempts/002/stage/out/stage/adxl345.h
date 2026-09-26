#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

#include <zephyr/drivers/spi.h>
struct device;

int adxl345_init(const struct device *dev);
int adxl345_read_xyz(const struct device *dev, int16_t *x, int16_t *y, int16_t *z);

#endif /* ADXL345_H */
