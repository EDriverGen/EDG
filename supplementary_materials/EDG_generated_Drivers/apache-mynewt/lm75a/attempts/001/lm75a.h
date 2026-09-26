#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>

#include <hal/hal_i2c.h>
struct lm75a_dev {
    uint8_t i2c_num;
    uint8_t i2c_addr;
};

int lm75a_init(struct lm75a_dev *dev, void *bus_handle);
int lm75a_read_temperature(struct lm75a_dev *dev, int32_t *raw);

#endif /* LM75A_H */
