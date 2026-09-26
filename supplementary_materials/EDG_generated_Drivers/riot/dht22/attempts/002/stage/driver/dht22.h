#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include <stdbool.h>
#include <periph/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dht22_device {
    gpio_t pin;
};

int dht22_init(struct dht22_device *dev, gpio_t pin);
int dht22_read(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val);

#ifdef __cplusplus
}
#endif

#endif /* DHT22_H */