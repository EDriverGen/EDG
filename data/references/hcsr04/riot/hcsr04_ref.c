/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for RIOT
 */
#include "hcsr04_ref.h"

int hcsr04_init(struct hcsr04_device *dev, gpio_t trig, gpio_t echo)
{
  if (dev == NULL) return -1;

  dev->trig = trig;
  dev->echo = echo;

  if (gpio_init(trig, GPIO_OUT) < 0) return -1;
  if (gpio_init(echo, GPIO_IN) < 0) return -1;
  gpio_clear(trig);

  return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
  uint32_t timeout;
  uint32_t elapsed;

  if (dev == NULL || distance_mm == NULL) return -1;

  gpio_set(dev->trig);
  xtimer_usleep(10);
  gpio_clear(dev->trig);

  timeout = HCSR04_TIMEOUT_US;
  while (gpio_read(dev->echo) == 0 && timeout > 0)
    { xtimer_usleep(1); timeout--; }
  if (timeout == 0) return -1;

  elapsed = 0;
  while (gpio_read(dev->echo) != 0 && elapsed < HCSR04_TIMEOUT_US)
    { xtimer_usleep(1); elapsed++; }

  *distance_mm = (int32_t)(elapsed * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);
  return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}
