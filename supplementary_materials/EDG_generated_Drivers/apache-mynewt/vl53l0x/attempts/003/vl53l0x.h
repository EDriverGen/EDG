#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>

#include <hal/hal_i2c.h>
struct vl53l0x_dev {
    uint8_t i2c_num;
    uint8_t i2c_addr;
};

int vl53l0x_init(struct vl53l0x_dev *dev, void *bus_handle);
int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw);

#endif
