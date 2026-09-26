#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

struct dht22_device {
    uint64_t pin;
};

int dht22_init(struct dht22_device *dev, uint64_t pin);
int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature);

#endif /* DHT22_H */