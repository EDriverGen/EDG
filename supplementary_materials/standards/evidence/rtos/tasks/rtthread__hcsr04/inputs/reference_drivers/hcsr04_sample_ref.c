/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Sample for RT-Thread
 */
#include <rtthread.h>
#include "hcsr04_ref.h"

#define HCSR04_TRIG_PIN  GET_PIN(A, 0)
#define HCSR04_ECHO_PIN  GET_PIN(A, 1)

int hcsr04_sample(void)
{
  struct hcsr04_device dev;
  int32_t distance_mm;
  int i, ret;

  ret = hcsr04_init(&dev, HCSR04_TRIG_PIN, HCSR04_ECHO_PIN);
  if (ret < 0) { rt_kprintf("ERROR: hcsr04 init failed\n"); return -1; }

  rt_kprintf("[HC-SR04] initialized\n");

  for (i = 0; i < 5; i++)
    {
      ret = hcsr04_read_distance_mm(&dev, &distance_mm);
      if (ret < 0) { rt_kprintf("ERROR: read failed\n"); break; }

      rt_kprintf("[HC-SR04] sample=%d distance=%d.%d cm\n",
                 i + 1, (int)(distance_mm / 10), (int)(distance_mm % 10));

      rt_thread_mdelay(500);
    }

  return 0;
}
MSH_CMD_EXPORT(hcsr04_sample, hcsr04 ultrasonic distance test);
