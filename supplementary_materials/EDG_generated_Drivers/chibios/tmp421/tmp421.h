#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include "hal.h"

#define TMP421_I2C_ADDR 0x2A

#include "hal_i2c.h"
struct tmp421_device {
    I2CDriver *bus_handle;
    uint8_t i2c_addr;
};

int tmp421_init(struct tmp421_device *dev, void *bus_handle);
int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val);
int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val);

#endif
