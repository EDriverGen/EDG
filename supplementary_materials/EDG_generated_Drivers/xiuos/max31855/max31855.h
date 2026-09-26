#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>

struct max31855_dev {
    int fd;
};

int max31855_init(struct max31855_dev *dev, void *bus_handle);
int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc_val, int32_t *int_val);

#endif