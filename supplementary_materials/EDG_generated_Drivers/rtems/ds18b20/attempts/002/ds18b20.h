#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

struct ds18b20_dev {
    uint32_t data_pin;
};

int ds18b20_init(struct ds18b20_dev *dev, uint32_t data_pin);
int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw);

#endif