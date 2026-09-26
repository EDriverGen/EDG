#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>
#include "stm32f1xx_hal_gpio.h"

struct hcsr04_device {
    GPIO_TypeDef *trig_port;
    uint16_t trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev, void *bus_handle);
int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw);

#endif /* HCSR04_H */