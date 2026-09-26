#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include <stddef.h>

struct tmp421_device {
    void *bus_handle;
    uint8_t i2c_addr;
};

void tmp421_init(struct tmp421_device *dev, void *bus_handle);
int32_t tmp421_read_local(struct tmp421_device *dev, int32_t *val);
int32_t tmp421_read_remote(struct tmp421_device *dev, int32_t *val);

#endif /* TMP421_H */