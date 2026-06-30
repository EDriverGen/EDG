/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for Zephyr
 */
#include "hcsr04_ref.h"

int hcsr04_init(struct hcsr04_device *dev,
                const struct gpio_dt_spec *trig,
                const struct gpio_dt_spec *echo)
{
  if (dev == NULL || trig == NULL || echo == NULL) return -1;

  dev->trig = trig;
  dev->echo = echo;

  if (gpio_pin_configure_dt(trig, GPIO_OUTPUT_INACTIVE) < 0) return -1;
  if (gpio_pin_configure_dt(echo, GPIO_INPUT) < 0) return -1;

  return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
  uint32_t timeout;
  uint32_t elapsed;

  if (dev == NULL || distance_mm == NULL) return -1;

  gpio_pin_set_dt(dev->trig, 1);
  k_busy_wait(10);
  gpio_pin_set_dt(dev->trig, 0);

  timeout = HCSR04_TIMEOUT_US;
  while (gpio_pin_get_dt(dev->echo) == 0 && timeout > 0)
    { k_busy_wait(1); timeout--; }
  if (timeout == 0) return -1;

  elapsed = 0;
  while (gpio_pin_get_dt(dev->echo) == 1 && elapsed < HCSR04_TIMEOUT_US)
    { k_busy_wait(1); elapsed++; }

  *distance_mm = (int32_t)(elapsed * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);
  return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}
