#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include <stdbool.h>
#include <periph/gpio.h>

struct dht22_device {
    gpio_t pin;
};

int dht22_init(struct dht22_device *dev, gpio_t pin);
int dht22_read(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val);

#endif /* DHT22_H */