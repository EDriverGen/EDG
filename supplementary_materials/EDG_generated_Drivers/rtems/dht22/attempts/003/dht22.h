#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

struct dht22_device {
    uint32_t data_pin;
};

int dht22_init(struct dht22_device *dev, uint32_t data_pin);
int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature);

#endif /* DHT22_H */