#ifndef TMP105_H
#define TMP105_H

#include <stdint.h>
#include <rtdef.h>

struct tmp105_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int tmp105_init(struct tmp105_device *dev, struct rt_i2c_bus_device *bus);
int tmp105_read_temperature(struct tmp105_device *dev, int32_t *raw);

#endif /* TMP105_H */