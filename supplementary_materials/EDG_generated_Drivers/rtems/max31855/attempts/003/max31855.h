#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>

struct max31855_device {
    int fd;
};

int max31855_init(struct max31855_device *dev, int bus_handle);
int max31855_read_temperatures(struct max31855_device *dev, int32_t *thermocouple_val, int32_t *temp_local_val);

#endif /* MAX31855_H */