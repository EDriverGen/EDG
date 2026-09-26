#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>
#include "periph/i2c.h"

typedef struct {
    i2c_t bus;
    uint8_t addr;
    uint8_t mtreg;
} bh1750_device_t;

int bh1750_init(bh1750_device_t *dev, i2c_t bus);
int bh1750_read_illuminance(bh1750_device_t *dev, uint16_t *raw);

#endif /* BH1750_H */
