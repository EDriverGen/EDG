#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include <stddef.h>
#include "hal.h"

#include "hal_i2c.h"
struct vl53l0x_dev {
    I2CDriver *bus_handle;
    uint8_t i2c_addr;
};

int32_t vl53l0x_init(struct vl53l0x_dev *dev, void *bus_handle);
int32_t vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw);

#endif /* VL53L0X_H */
