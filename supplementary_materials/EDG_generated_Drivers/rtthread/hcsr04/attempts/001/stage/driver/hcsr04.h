#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

struct hcsr04_device {
    uint64_t trig_pin;
    uint64_t echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev, uint64_t trig_pin, uint64_t echo_pin);
int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *distance_mm);

#endif /* HCSR04_H */