#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>

struct tmp421_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int tmp421_init(struct tmp421_dev *dev, void *bus_handle);
int tmp421_read_local(struct tmp421_dev *dev, int32_t *temp_local_val);
int tmp421_read_remote(struct tmp421_dev *dev, int32_t *temp_remote_val);

#endif