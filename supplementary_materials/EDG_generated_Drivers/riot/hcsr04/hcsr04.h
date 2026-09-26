#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>
#include <periph/gpio.h>

typedef struct {
    gpio_t trig_pin;
    gpio_t echo_pin;
} hcsr04_t;

int hcsr04_init(hcsr04_t *dev, gpio_t trig, gpio_t echo);
int hcsr04_read_distance(hcsr04_t *dev, int32_t *raw);

#endif /* HCSR04_H */