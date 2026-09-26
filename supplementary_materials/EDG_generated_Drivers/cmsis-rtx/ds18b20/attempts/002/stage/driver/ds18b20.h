#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

struct ds18b20_dev {
    void *bus_handle;
};

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle);
int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw);

#endif