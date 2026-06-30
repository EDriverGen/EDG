/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for XiUOS
 */
#include "hcsr04_ref.h"
#include <stdio.h>

int hcsr04_init(struct hcsr04_device *dev,
                const char *trig_dev_path,
                const char *echo_dev_path)
{
  if (dev == NULL) return -1;

  dev->trig_fd = PrivOpen(trig_dev_path, O_RDWR);
  if (dev->trig_fd < 0) return -1;

  dev->echo_fd = PrivOpen(echo_dev_path, O_RDWR);
  if (dev->echo_fd < 0) { PrivClose(dev->trig_fd); return -1; }

  return 0;
}

void hcsr04_deinit(struct hcsr04_device *dev)
{
  if (dev != NULL)
    {
      if (dev->trig_fd >= 0) { PrivClose(dev->trig_fd); dev->trig_fd = -1; }
      if (dev->echo_fd >= 0) { PrivClose(dev->echo_fd); dev->echo_fd = -1; }
    }
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
  uint8_t val;
  uint32_t timeout;
  uint32_t start, now;

  if (dev == NULL || distance_mm == NULL) return -1;

  /* Trigger pulse */
  val = 1;
  PrivWrite(dev->trig_fd, &val, 1);
  hcsr04_delay_us(10);
  val = 0;
  PrivWrite(dev->trig_fd, &val, 1);

  /* Wait for echo HIGH */
  timeout = HCSR04_TIMEOUT_US;
  do {
    PrivRead(dev->echo_fd, &val, 1);
    hcsr04_delay_us(1);
    timeout--;
  } while (val == 0 && timeout > 0);
  if (timeout == 0) return -1;

  /* Measure echo duration */
  start = hcsr04_get_us_tick();
  do {
    PrivRead(dev->echo_fd, &val, 1);
    now = hcsr04_get_us_tick();
  } while (val != 0 && (now - start) < HCSR04_TIMEOUT_US);

  now = hcsr04_get_us_tick();
  *distance_mm = (int32_t)((now - start) * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);
  return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}
