#ifndef TMP105_H
#define TMP105_H

#include <stdint.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} tmp105_t;

int tmp105_init(tmp105_t *dev, i2c_t bus, uint8_t addr);
int tmp105_read_temperature(tmp105_t *dev, int32_t *raw);

#endif /* TMP105_H */
