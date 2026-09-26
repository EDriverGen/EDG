#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>

#include "openharmony_liteosm.h"
struct bh1750_device {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int32_t bh1750_init(struct bh1750_device *dev, DevHandle bus_handle);
int32_t bh1750_read_illuminance(struct bh1750_device *dev, int32_t *raw);

#endif
