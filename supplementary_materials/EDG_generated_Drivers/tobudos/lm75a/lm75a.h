#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>
#include "tobudos.h"

struct lm75a_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int lm75a_init(struct lm75a_dev *dev, void *bus_handle);
int lm75a_read_temp(struct lm75a_dev *dev, int32_t *raw);

#endif /* LM75A_H */