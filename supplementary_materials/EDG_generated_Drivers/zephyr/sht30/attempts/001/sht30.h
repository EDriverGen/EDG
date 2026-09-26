#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
struct device;

int sht30_init(const struct device *dev);
int sht30_read_measurement(const struct device *dev, int32_t *temp_val, int32_t *hum_val);

#endif /* SHT30_H */
