#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>

struct vl53l0x_dev {
    int fd;
    uint8_t i2c_addr;
};

int vl53l0x_init(struct vl53l0x_dev *dev, int bus_handle);
int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw);

#endif /* VL53L0X_H */