#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>

struct sht30_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int sht30_init(struct sht30_device *dev, struct rt_i2c_bus_device *bus);
int sht30_read_temperature(struct sht30_device *dev, int32_t *temperature_milliC);
int sht30_read_humidity(struct sht30_device *dev, int32_t *humidity_milliPercent);

#endif
