#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>

struct bh1750_ctx {
    void *bus_handle;
    uint8_t i2c_addr;
};

int bh1750_init(struct bh1750_ctx *dev, void *bus_handle);
int bh1750_read_illuminance(struct bh1750_ctx *dev, int32_t *raw);

#endif