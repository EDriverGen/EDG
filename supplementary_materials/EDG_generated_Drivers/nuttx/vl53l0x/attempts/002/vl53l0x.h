#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

struct i2c_master_s;

struct vl53l0x_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int vl53l0x_init(struct vl53l0x_dev_s *dev, struct i2c_master_s *bus);
int vl53l0x_read_distance(struct vl53l0x_dev_s *dev, int32_t *raw);

#endif /* VL53L0X_H */