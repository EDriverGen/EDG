#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>

struct emc1413_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int emc1413_init(struct emc1413_device *dev, struct rt_i2c_bus_device *bus);
int emc1413_read_internal_temp(struct emc1413_device *dev, int32_t *temp);
int emc1413_read_external_diode_1_temp(struct emc1413_device *dev, int32_t *temp);
int emc1413_read_external_diode_2_temp(struct emc1413_device *dev, int32_t *temp);

#endif /* EMC1413_H */