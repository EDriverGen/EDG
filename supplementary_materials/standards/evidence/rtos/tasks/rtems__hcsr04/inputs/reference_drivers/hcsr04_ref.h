#ifndef HCSR04_RTEMS_REF_H
#define HCSR04_RTEMS_REF_H

#include <stdint.h>
#include <rtems.h>
#include <rtems/gpio.h>

#define HCSR04_MAX_DISTANCE_CM 400
#define HCSR04_TIMEOUT_US      25000U

struct hcsr04_device {
    rtems_gpio_pin trig_pin;
    rtems_gpio_pin echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev, rtems_gpio_pin trig_pin, rtems_gpio_pin echo_pin);
int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm);

#define HCSR04_STATUS_OK         0
#define HCSR04_STATUS_TIMEOUT    1
#define HCSR04_STATUS_OVERRANGE  2
#define HCSR04_STATUS_BELOW_MIN  3
#define HCSR04_STATUS_ABOVE_MAX  4
int hcsr04_decode_distance(unsigned long echo_us, int *cm_x10, int *status_code);

#endif
