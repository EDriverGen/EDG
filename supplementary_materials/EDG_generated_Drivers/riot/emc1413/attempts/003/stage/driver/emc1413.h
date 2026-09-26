#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint16_t addr;
} emc1413_t;

int emc1413_init(emc1413_t *dev, i2c_t bus);
int emc1413_read_internal_temperature(emc1413_t *dev, int32_t *temp_local_val);
int emc1413_read_external_diode_1_temperature(emc1413_t *dev, int32_t *temp_ext1_val);
int emc1413_read_external_diode_2_temperature(emc1413_t *dev, int32_t *temp_ext2_val);

#endif /* EMC1413_H */
