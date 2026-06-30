/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Sample for RIOT
 */
#include <stdio.h>
#include "periph/gpio.h"
#include "xtimer.h"
#include "hcsr04_ref.h"

#define HCSR04_TRIG_PIN  GPIO_PIN(0, 0)
#define HCSR04_ECHO_PIN  GPIO_PIN(0, 1)

int main(void)
{
  struct hcsr04_device dev;
  int32_t distance_mm;
  int i, ret;

  ret = hcsr04_init(&dev, HCSR04_TRIG_PIN, HCSR04_ECHO_PIN);
  if (ret < 0) { printf("ERROR: hcsr04 init failed\n"); return -1; }

  printf("[HC-SR04] initialized\n");

  for (i = 0; i < 5; i++)
    {
      ret = hcsr04_read_distance_mm(&dev, &distance_mm);
      if (ret < 0) { printf("ERROR: read failed\n"); break; }

      printf("[HC-SR04] sample=%d distance=%d.%d cm\n",
             i + 1, (int)(distance_mm / 10), (int)(distance_mm % 10));

      xtimer_sleep(1);
    }

  return 0;
}
