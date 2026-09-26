#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>

struct bh1750_device {
    int fd;
    uint8_t i2c_addr;
};

int bh1750_init(struct bh1750_device *dev, void *bus_handle);
int bh1750_read_illuminance(struct bh1750_device *dev, int32_t *raw);

#endif
