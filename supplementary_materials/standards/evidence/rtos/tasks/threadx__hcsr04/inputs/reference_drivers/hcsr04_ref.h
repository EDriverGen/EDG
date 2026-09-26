/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for ThreadX
 */
#ifndef __HCSR04_REF_H
#define __HCSR04_REF_H

#include <stdint.h>
#include <stddef.h>
#include <tx_api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HCSR04_SPEED_OF_SOUND_CM_US  0.0343f
#define HCSR04_MAX_DISTANCE_CM       400
#define HCSR04_TIMEOUT_US            25000

struct hcsr04_platform_ops
{
  void (*gpio_set_output)(int port, int pin);
  void (*gpio_set_input)(int port, int pin);
  void (*gpio_write)(int port, int pin, int value);
  int  (*gpio_read)(int port, int pin);
  void (*delay_us)(uint32_t us);
  uint32_t (*get_us_tick)(void);
};

struct hcsr04_device
{
  const struct hcsr04_platform_ops *ops;
  int trig_port;
  int trig_pin;
  int echo_port;
  int echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev,
                const struct hcsr04_platform_ops *ops,
                int trig_port, int trig_pin,
                int echo_port, int echo_pin);
int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm);

#ifdef __cplusplus
}
#endif

#endif /* __HCSR04_REF_H */