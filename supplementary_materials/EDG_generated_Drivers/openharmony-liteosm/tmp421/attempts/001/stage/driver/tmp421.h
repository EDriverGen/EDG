#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include "i2c_if.h"

#define TMP421_I2C_ADDR 0x2A

#include "openharmony_liteosm.h"
struct tmp421_device {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int32_t tmp421_init(struct tmp421_device *dev, DevHandle bus_handle);
int32_t tmp421_read_temp_local(struct tmp421_device *dev, int32_t *temp_local);
int32_t tmp421_read_temp_remote(struct tmp421_device *dev, int32_t *temp_remote);

#endif
