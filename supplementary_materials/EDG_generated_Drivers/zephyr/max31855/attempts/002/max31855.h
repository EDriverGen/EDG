#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>

#include <zephyr/drivers/spi.h>
struct device;

int max31855_init(const struct device *dev);
int max31855_read_temperatures(const struct device *dev, int32_t *thermocouple_val, int32_t *temp_local_val);

#endif /* MAX31855_H */
