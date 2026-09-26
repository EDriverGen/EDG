#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>

#include <hal/hal_i2c.h>
struct bh1750_dev {
    uint8_t i2c_num;
    uint8_t i2c_addr;
};

int bh1750_init(struct bh1750_dev *dev, void *bus_handle);
int bh1750_read(struct bh1750_dev *dev, int32_t *raw);

#endif
