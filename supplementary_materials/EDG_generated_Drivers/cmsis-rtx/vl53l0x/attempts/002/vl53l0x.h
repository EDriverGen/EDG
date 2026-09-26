#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    void *bus_handle;
    uint8_t i2c_addr;
} vl53l0x_t;

int32_t vl53l0x_init(vl53l0x_t *dev, void *bus_handle);
int32_t vl53l0x_read_distance(vl53l0x_t *dev, int32_t *raw);

#endif /* VL53L0X_H */