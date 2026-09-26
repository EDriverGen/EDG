#ifndef TMP105_H
#define TMP105_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
struct device;

int tmp105_init(const struct device *dev);
int tmp105_read_temperature(const struct device *dev, int32_t *raw);

#endif /* TMP105_H */
