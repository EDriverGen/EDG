#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

struct dht22_device {
    uint16_t gpio_pin;
};

int32_t dht22_init(struct dht22_device *dev, uint16_t gpio_pin);
int32_t dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val);

#endif /* DHT22_H */