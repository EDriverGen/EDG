/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for Zephyr
 */
#ifndef __HCSR04_REF_H
#define __HCSR04_REF_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HCSR04_SPEED_OF_SOUND_CM_US  0.0343f
#define HCSR04_MAX_DISTANCE_CM       400
#define HCSR04_TIMEOUT_US            25000

struct hcsr04_device
{
  const struct gpio_dt_spec *trig;
  const struct gpio_dt_spec *echo;
};

int hcsr04_init(struct hcsr04_device *dev,
                const struct gpio_dt_spec *trig,
                const struct gpio_dt_spec *echo);
int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm);

#ifdef __cplusplus
}
#endif

#endif /* __HCSR04_REF_H */
