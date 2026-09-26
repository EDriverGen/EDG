#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>
#include <stddef.h>

#define EMC1413_I2C_ADDR 0x4C

struct emc1413_dev {
    uint8_t i2c_addr;
    uint8_t bus_num;
};

int emc1413_init(struct emc1413_dev *dev, void *bus_handle);
int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp);
int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp);
int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp);

#endif