#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include <stddef.h>
#include "periph/i2c.h"

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} vl53l0x_t;

int vl53l0x_init(vl53l0x_t *dev, i2c_t bus);
int vl53l0x_read_distance(vl53l0x_t *dev, int32_t *raw);

#endif
