#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

struct hcsr04_device {
    uint16_t trig_pin;
    uint16_t echo_pin;
};

int32_t hcsr04_init(struct hcsr04_device *dev, uint16_t trig_pin, uint16_t echo_pin);
int32_t hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw);

#endif /* HCSR04_H */