#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include <rtdevice.h>

struct vl53l0x_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int vl53l0x_init(struct vl53l0x_device *dev, struct rt_i2c_bus_device *bus);
int vl53l0x_read_distance(struct vl53l0x_device *dev, uint16_t *raw);

#endif /* VL53L0X_H */