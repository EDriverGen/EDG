#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
struct device;

int vl53l0x_init(const struct device *dev);
int vl53l0x_read_distance(const struct device *dev, int32_t *raw);

#endif /* VL53L0X_H */
