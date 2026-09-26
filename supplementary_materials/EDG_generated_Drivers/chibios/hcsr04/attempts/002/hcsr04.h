#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>
#include <stddef.h>

struct hcsr04_device {
    void *port;
    uint16_t trig_pin;
    uint16_t echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev);
int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw);

#endif
