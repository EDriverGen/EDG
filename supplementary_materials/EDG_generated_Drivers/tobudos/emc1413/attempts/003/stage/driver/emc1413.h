#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>
#include <stddef.h>

struct emc1413_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int emc1413_init(struct emc1413_dev *dev, void *bus_handle);
int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp);
int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp);
int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp);

#endif