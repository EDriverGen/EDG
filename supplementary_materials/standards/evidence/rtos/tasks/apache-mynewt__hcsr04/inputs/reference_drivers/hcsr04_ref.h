#ifndef HCSR04_APACHE_MYNEWT_REF_H
#define HCSR04_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_gpio.h"
#include <stdint.h>

#define HCSR04_MAX_DISTANCE_CM 400
#define HCSR04_TIMEOUT_US      25000U

struct hcsr04_device {
    int trig_pin;
    int echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev, int trig_pin, int echo_pin);
int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm);

#define HCSR04_STATUS_OK         0
#define HCSR04_STATUS_TIMEOUT    1
#define HCSR04_STATUS_OVERRANGE  2
#define HCSR04_STATUS_BELOW_MIN  3
#define HCSR04_STATUS_ABOVE_MAX  4
int hcsr04_decode_distance(unsigned long echo_us, int *cm_x10, int *status_code);

#endif
