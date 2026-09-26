#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

struct ds18b20_device {
    uint64_t pin;
};

int ds18b20_init(struct ds18b20_device *dev, uint64_t pin);
int ds18b20_read_temperature(struct ds18b20_device *dev, int32_t *raw);

#endif /* DS18B20_H */