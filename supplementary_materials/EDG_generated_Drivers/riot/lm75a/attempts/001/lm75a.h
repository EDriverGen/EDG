#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>
#include "periph/i2c.h"

typedef struct {
    i2c_t bus;
    uint16_t addr;
} lm75a_t;

int lm75a_init(lm75a_t *dev, i2c_t bus, uint16_t addr);
int lm75a_read_temperature(lm75a_t *dev, int32_t *raw);

#endif /* LM75A_H */