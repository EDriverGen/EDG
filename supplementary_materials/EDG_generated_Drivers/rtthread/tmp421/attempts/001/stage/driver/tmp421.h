#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include <rtdef.h>

struct tmp421_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int tmp421_init(struct tmp421_device *dev, struct rt_i2c_bus_device *bus);
int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_milli);
int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_milli);

#endif /* TMP421_H */