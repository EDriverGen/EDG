#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>

struct tmp421_device {
    int fd;
    uint8_t addr;
};

int tmp421_init(struct tmp421_device *dev, void *bus_handle);
int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *val);
int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *val);

#endif