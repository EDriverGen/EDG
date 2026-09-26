#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>
#include <periph/gpio.h>

typedef struct {
    gpio_t pin;
} ds18b20_t;

int ds18b20_init(ds18b20_t *dev, gpio_t pin);
int ds18b20_read_temperature(ds18b20_t *dev, int32_t *raw);

#endif /* DS18B20_H */