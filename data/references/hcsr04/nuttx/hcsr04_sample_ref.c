/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Sample for NuttX
 */
#include <stdio.h>
#include <unistd.h>
#include "hcsr04_ref.h"

int hcsr04_sample_main(int argc, FAR char *argv[])
{
  struct hcsr04_device dev;
  int32_t distance_mm;
  int i, ret;

  ret = hcsr04_init(&dev, "/dev/gpio0", "/dev/gpio1");
  if (ret < 0) { printf("ERROR: hcsr04 init failed\n"); return -1; }

  printf("[HC-SR04] initialized\n");

  for (i = 0; i < 5; i++)
    {
      ret = hcsr04_read_distance_mm(&dev, &distance_mm);
      if (ret < 0) { printf("ERROR: read failed\n"); break; }

      printf("[HC-SR04] sample=%d distance=%d.%d cm\n",
             i + 1, (int)(distance_mm / 10), (int)(distance_mm % 10));

      usleep(500000);
    }

  hcsr04_deinit(&dev);
  return 0;
}
