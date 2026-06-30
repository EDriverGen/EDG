#ifndef VL53L0X_APACHE_MYNEWT_REF_H
#define VL53L0X_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_i2c.h"
#include <stdint.h>

#define VL53L0X_ADDR_DEFAULT 0x29
#define VL53L0X_MODEL_ID     0xEE

struct vl53l0x_device {
    uint8_t i2c_num;
    uint16_t addr;
};

int vl53l0x_init(struct vl53l0x_device *dev, uint8_t i2c_num, uint16_t addr);
int vl53l0x_probe(struct vl53l0x_device *dev);
int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm);

#endif
