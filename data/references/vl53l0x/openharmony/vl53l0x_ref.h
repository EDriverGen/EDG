#ifndef __VL53L0X_REF_H
#define __VL53L0X_REF_H
#include "i2c_if.h"
#include "osal_time.h"
#include <stdint.h>

#define VL53L0X_ADDR_DEFAULT  0x29
#define VL53L0X_MODEL_ID      0xEE

struct vl53l0x_device {
    DevHandle bus;
    uint16_t addr;
};

int vl53l0x_init(struct vl53l0x_device *dev, DevHandle bus, uint16_t addr);
int vl53l0x_probe(struct vl53l0x_device *dev);
int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm);
#endif
