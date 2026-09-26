#ifndef HCSR04_H
#define HCSR04_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

int hcsr04_init(void);
int hcsr04_read_distance(int32_t *raw);

#endif /* HCSR04_H */