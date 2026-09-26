#ifndef TMP105_H
#define TMP105_H

#include <stdint.h>
#include "i2c_if.h"

#include "openharmony_liteosm.h"
struct tmp105_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int32_t tmp105_init(struct tmp105_dev *dev, DevHandle bus_handle);
int32_t tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw);

#endif
