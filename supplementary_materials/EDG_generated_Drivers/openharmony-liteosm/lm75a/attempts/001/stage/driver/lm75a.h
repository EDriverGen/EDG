#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>
#include "openharmony_liteosm.h"

struct lm75a_device {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int32_t lm75a_init(struct lm75a_device *dev, DevHandle bus_handle);
int32_t lm75a_read_temp(struct lm75a_device *dev, int32_t *raw);

#endif /* LM75A_H */