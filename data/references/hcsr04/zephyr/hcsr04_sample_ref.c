/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include "hcsr04_ref.h"

static const struct gpio_dt_spec trig_gpio =
    GPIO_DT_SPEC_GET(DT_NODELABEL(hcsr04_trig), gpios);
static const struct gpio_dt_spec echo_gpio =
    GPIO_DT_SPEC_GET(DT_NODELABEL(hcsr04_echo), gpios);

int hcsr04_sample(void)
{
  struct hcsr04_device dev;
  int32_t distance_mm;
  int i, ret;

  ret = hcsr04_init(&dev, &trig_gpio, &echo_gpio);
  if (ret < 0) { printf("ERROR: hcsr04 init failed\n"); return -1; }

  printf("[HC-SR04] initialized\n");

  for (i = 0; i < 5; i++)
    {
      ret = hcsr04_read_distance_mm(&dev, &distance_mm);
      if (ret < 0) { printf("ERROR: read failed\n"); break; }

      printf("[HC-SR04] sample=%d distance=%d.%d cm\n",
             i + 1, (int)(distance_mm / 10), (int)(distance_mm % 10));

      k_msleep(500);
    }

  return 0;
}
