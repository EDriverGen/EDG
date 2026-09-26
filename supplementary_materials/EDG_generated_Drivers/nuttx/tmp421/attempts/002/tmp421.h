#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>

struct i2c_master_s;

struct tmp421_dev {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int tmp421_init(struct tmp421_dev *dev, struct i2c_master_s *bus);
int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *val);
int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *val);

#endif