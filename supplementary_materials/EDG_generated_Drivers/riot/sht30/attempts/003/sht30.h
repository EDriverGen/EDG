#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>
#include <stddef.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} sht30_t;

void sht30_init(sht30_t *dev, i2c_t bus);
int sht30_read_single_shot(sht30_t *dev, int32_t *temp_val, int32_t *hum_val);

#endif /* SHT30_H */
