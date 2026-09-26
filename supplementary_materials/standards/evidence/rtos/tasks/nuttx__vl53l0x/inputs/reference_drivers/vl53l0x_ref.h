#ifndef __VL53L0X_REF_H
#define __VL53L0X_REF_H
#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#define VL53L0X_ADDR_DEFAULT  0x29
#define VL53L0X_MODEL_ID      0xEE

struct vl53l0x_device {
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
};

int vl53l0x_init(struct vl53l0x_device *dev, FAR struct i2c_master_s *i2c, uint16_t addr);
int vl53l0x_probe(struct vl53l0x_device *dev);
int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm);
#endif
