#ifndef HCSR04_H
#define HCSR04_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

struct device;

int hcsr04_init(const struct device *dev);
int hcsr04_read_distance(const struct device *dev, int32_t *raw);

#endif /* HCSR04_H */