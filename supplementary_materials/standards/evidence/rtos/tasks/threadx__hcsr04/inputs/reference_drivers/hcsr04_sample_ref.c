/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Sample for ThreadX
 */
#include <stdio.h>
#include <tx_api.h>
#include "hcsr04_ref.h"

extern const struct hcsr04_platform_ops g_hcsr04_platform_ops;

void hcsr04_sample_entry(ULONG input)
{
  struct hcsr04_device dev;
  int32_t distance_mm;
  int i, ret;

  (void)input;

  ret = hcsr04_init(&dev, &g_hcsr04_platform_ops, 0, 0, 0, 1);
  if (ret < 0) { printf("ERROR: hcsr04 init failed
"); return; }

  printf("[HC-SR04] initialized
");

  for (i = 0; i < 5; i++)
    {
      ret = hcsr04_read_distance_mm(&dev, &distance_mm);
      if (ret < 0) { printf("ERROR: read failed
"); break; }

      printf("[HC-SR04] sample=%d distance=%d.%d cm
",
             i + 1, (int)(distance_mm / 10), (int)(distance_mm % 10));

      tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);
    }
}
