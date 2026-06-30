/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for NuttX
 */
#include "hcsr04_ref.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/ioexpander/gpio.h>
#include <nuttx/clock.h>

int hcsr04_init(struct hcsr04_device *dev,
                const char *trig_path, const char *echo_path)
{
  if (dev == NULL) return -1;

  dev->trig_fd = open(trig_path, O_RDWR);
  if (dev->trig_fd < 0) return -1;

  dev->echo_fd = open(echo_path, O_RDONLY);
  if (dev->echo_fd < 0) { close(dev->trig_fd); return -1; }

  return 0;
}

void hcsr04_deinit(struct hcsr04_device *dev)
{
  if (dev != NULL)
    {
      if (dev->trig_fd >= 0) close(dev->trig_fd);
      if (dev->echo_fd >= 0) close(dev->echo_fd);
    }
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
  bool val;
  uint32_t elapsed;
  uint32_t timeout;

  if (dev == NULL || distance_mm == NULL) return -1;

  /* Trigger pulse */
  val = true;
  ioctl(dev->trig_fd, GPIOC_WRITE, (unsigned long)&val);
  usleep(10);
  val = false;
  ioctl(dev->trig_fd, GPIOC_WRITE, (unsigned long)&val);

  /* Wait for echo */
  timeout = HCSR04_TIMEOUT_US;
  do { ioctl(dev->echo_fd, GPIOC_READ, (unsigned long)&val); usleep(1); timeout--; }
  while (!val && timeout > 0);
  if (timeout == 0) return -1;

  elapsed = 0;
  do { ioctl(dev->echo_fd, GPIOC_READ, (unsigned long)&val); usleep(1); elapsed++; }
  while (val && elapsed < HCSR04_TIMEOUT_US);

  *distance_mm = (int32_t)(elapsed * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);
  return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}
