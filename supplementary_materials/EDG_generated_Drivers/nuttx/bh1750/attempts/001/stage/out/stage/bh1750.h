#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

struct bh1750_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int bh1750_init(struct bh1750_dev_s *dev, struct i2c_master_s *bus);
int bh1750_read_illuminance(struct bh1750_dev_s *dev, int32_t *raw);

#endif /* BH1750_H */
