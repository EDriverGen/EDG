#ifndef __HCSR04_REF_H
#define __HCSR04_REF_H

#include "gpio_if.h"
#include "osal_time.h"
#include <stdint.h>

#define HCSR04_SPEED_OF_SOUND_CM_US  0.0343f
#define HCSR04_MAX_DISTANCE_CM       400
#define HCSR04_TIMEOUT_US            25000

struct hcsr04_device {
    uint16_t trig_gpio;
    uint16_t echo_gpio;
};

int hcsr04_init(struct hcsr04_device *dev, uint16_t trig_gpio, uint16_t echo_gpio);
int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm);

#endif
