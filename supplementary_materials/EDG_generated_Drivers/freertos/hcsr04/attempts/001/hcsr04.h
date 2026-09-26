#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>
#include "stm32f1xx_hal_gpio.h"

struct hcsr04_dev {
    GPIO_TypeDef *gpio_port;
    uint16_t trig_pin;
    uint16_t echo_pin;
};

int hcsr04_init(struct hcsr04_dev *dev, GPIO_TypeDef *port, uint16_t trig, uint16_t echo);
int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw);

#endif /* HCSR04_H */