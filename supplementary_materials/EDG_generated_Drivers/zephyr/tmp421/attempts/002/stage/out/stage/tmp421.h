#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include <zephyr/device.h>

#include <zephyr/drivers/i2c.h>
struct device;

int tmp421_init(const struct device *dev);
int tmp421_read_temperature_local(const struct device *dev, int32_t *temp_local_val);
int tmp421_read_temperature_remote1(const struct device *dev, int32_t *temp_remote_val);

#endif /* TMP421_H */
