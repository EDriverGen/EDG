#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>

struct max31855_device {
    struct rt_spi_device *spi;
};

int max31855_init(struct max31855_device *dev, struct rt_spi_device *spi);
int max31855_read_thermocouple(struct max31855_device *dev, int32_t *tc);
int max31855_read_internal(struct max31855_device *dev, int32_t *internal);

#endif
