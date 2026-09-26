#ifndef TMP105_H
#define TMP105_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

struct i2c_master_s;

struct tmp105_dev {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int tmp105_init(struct tmp105_dev *dev, struct i2c_master_s *bus);
int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *temp);

#endif /* TMP105_H */