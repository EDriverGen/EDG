#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include <stddef.h>

struct tmp421_device {
    void *bus_handle;
    uint8_t i2c_addr;
};

int tmp421_init(struct tmp421_device *dev, void *bus_handle);
int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *temp);
int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *temp);

#endif