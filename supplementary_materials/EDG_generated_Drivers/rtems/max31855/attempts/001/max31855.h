#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>

#include <dev/spi/spi.h>
struct max31855_device {
    spi_bus bus;
};

int max31855_init(struct max31855_device *dev, spi_bus bus);
int max31855_read_temperatures(struct max31855_device *dev, int32_t *thermocouple_val, int32_t *temp_local_val);

#endif /* MAX31855_H */
