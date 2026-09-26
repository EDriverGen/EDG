#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include "bus_i2c.h"

struct I2cBus;

struct vl53l0x_dev {
    struct I2cBus *bus;
    int fd;
    uint8_t i2c_addr;
};

int vl53l0x_init(struct vl53l0x_dev *dev, struct I2cBus *bus);
int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw);

#endif