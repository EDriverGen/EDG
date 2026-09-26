#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

struct hcsr04_dev {
    int trig_pin;
    int echo_pin;
};

int hcsr04_init(struct hcsr04_dev *dev, int trig_pin, int echo_pin);
int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw);

#endif