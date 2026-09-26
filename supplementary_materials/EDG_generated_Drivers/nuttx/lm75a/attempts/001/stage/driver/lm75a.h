#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>

struct i2c_master_s;

struct lm75a_dev {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int lm75a_init(struct lm75a_dev *dev, struct i2c_master_s *bus);
int lm75a_read_temp(struct lm75a_dev *dev, int32_t *raw);

#endif /* LM75A_H */
