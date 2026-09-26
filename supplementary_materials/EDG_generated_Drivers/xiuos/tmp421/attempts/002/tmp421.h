#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include <stddef.h>
#include "bus_i2c.h"

struct I2cBus;

struct tmp421_device {
    struct I2cBus *bus;
    int fd;
    uint8_t addr;
};

int tmp421_init(struct tmp421_device *dev, struct I2cBus *bus_handle);
int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val);
int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val);

#endif