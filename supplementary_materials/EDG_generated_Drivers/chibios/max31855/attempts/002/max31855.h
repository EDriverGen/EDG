#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>
#include <stddef.h>

struct max31855_device {
    void *bus_handle;
};

void max31855_init(struct max31855_device *dev, void *bus_handle);
int max31855_read_temperatures(struct max31855_device *dev, int32_t *tc, int32_t *local);

#endif