/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for ThreadX
 */
#include "hcsr04_ref.h"

int hcsr04_init(struct hcsr04_device *dev,
                const struct hcsr04_platform_ops *ops,
                int trig_port, int trig_pin,
                int echo_port, int echo_pin)
{
  if (dev == NULL || ops == NULL) return -1;

  dev->ops = ops;
  dev->trig_port = trig_port;
  dev->trig_pin = trig_pin;
  dev->echo_port = echo_port;
  dev->echo_pin = echo_pin;

  if (ops->gpio_set_output == NULL || ops->gpio_set_input == NULL || ops->gpio_write == NULL) return -1;
  ops->gpio_set_output(trig_port, trig_pin);
  ops->gpio_set_input(echo_port, echo_pin);
  ops->gpio_write(trig_port, trig_pin, 0);

  return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
  uint32_t start, now;
  uint32_t timeout;

  if (dev == NULL || distance_mm == NULL || dev->ops == NULL) return -1;
  if (dev->ops->gpio_write == NULL || dev->ops->gpio_read == NULL ||
      dev->ops->delay_us == NULL || dev->ops->get_us_tick == NULL) return -1;

  dev->ops->gpio_write(dev->trig_port, dev->trig_pin, 1);
  dev->ops->delay_us(10);
  dev->ops->gpio_write(dev->trig_port, dev->trig_pin, 0);

  timeout = HCSR04_TIMEOUT_US;
  while (dev->ops->gpio_read(dev->echo_port, dev->echo_pin) == 0 && timeout > 0)
    { dev->ops->delay_us(1); timeout--; }
  if (timeout == 0) return -1;

  start = dev->ops->get_us_tick();
  while (dev->ops->gpio_read(dev->echo_port, dev->echo_pin) == 1)
    {
      now = dev->ops->get_us_tick();
      if ((now - start) > HCSR04_TIMEOUT_US) return -1;
    }
  now = dev->ops->get_us_tick();

  *distance_mm = (int32_t)((now - start) * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);
  return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}
