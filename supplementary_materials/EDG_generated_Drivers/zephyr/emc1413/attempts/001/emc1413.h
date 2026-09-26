#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

#define EMC1413_I2C_ADDR 0x4C

#include <zephyr/device.h>
struct device;

int emc1413_init(const struct device *dev);
int emc1413_read_internal_temperature(const struct device *dev, int32_t *temp_local_val);
int emc1413_read_external_diode_1_temperature(const struct device *dev, int32_t *temp_ext1_val);
int emc1413_read_external_diode_2_temperature(const struct device *dev, int32_t *temp_ext2_val);

#endif /* EMC1413_H */
