#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

struct hcsr04_device {
    uint32_t trig_pin;
    uint32_t echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev, uint32_t trig_pin, uint32_t echo_pin);
int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw);

#endif /* HCSR04_H */