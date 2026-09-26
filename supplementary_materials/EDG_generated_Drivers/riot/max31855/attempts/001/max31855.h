#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>

#include "riot.h"
typedef struct {
    spi_t bus;
    spi_cs_t cs;
} max31855_t;

int max31855_init(max31855_t *dev, spi_t bus, spi_cs_t cs);
int max31855_read_temperatures(max31855_t *dev, int32_t *thermocouple_val, int32_t *internal_val);

#endif /* MAX31855_H */
