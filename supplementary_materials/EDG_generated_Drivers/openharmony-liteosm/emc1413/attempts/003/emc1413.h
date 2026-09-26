#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>
#include "i2c_if.h"

#define EMC1413_I2C_ADDR 0x4C

#include "openharmony_liteosm.h"
struct emc1413_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int32_t emc1413_init(struct emc1413_dev *dev, DevHandle bus_handle);
int32_t emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val);
int32_t emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val);
int32_t emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val);

#endif
