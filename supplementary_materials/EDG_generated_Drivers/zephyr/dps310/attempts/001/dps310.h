#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
struct device;

int dps310_init(const struct device *dev);
int dps310_read_pressure(const struct device *dev, int32_t *pressure_raw);
int dps310_read_temp(const struct device *dev, int32_t *temp_raw);

#endif /* DPS310_H */
