#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>

#define EMC1413_I2C_ADDR 0x4C

#include "apache_mynewt.h"
struct emc1413_dev {
    uint8_t i2c_num;
};

int emc1413_init(struct emc1413_dev *dev, void *bus_handle);
int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val);
int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val);
int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val);

#endif
