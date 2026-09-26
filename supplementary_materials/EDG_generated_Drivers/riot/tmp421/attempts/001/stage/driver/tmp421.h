#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include "periph/i2c.h"

#include "riot.h"
struct tmp421_device {
    i2c_t bus;
    uint16_t addr;
};

int tmp421_init(struct tmp421_device *dev, i2c_t bus, uint16_t addr);
int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val);
int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val);

#endif /* TMP421_H */
