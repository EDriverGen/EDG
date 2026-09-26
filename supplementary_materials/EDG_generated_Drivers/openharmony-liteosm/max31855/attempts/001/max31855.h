#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>
#include "spi_if.h"

struct max31855_device {
    DevHandle spi_handle;
};

int32_t max31855_init(struct max31855_device *dev, DevHandle bus_handle);
int32_t max31855_read_temperatures(struct max31855_device *dev, int32_t *tc, int32_t *local);

#endif