/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for RT-Thread
 */
#ifndef __HCSR04_REF_H
#define __HCSR04_REF_H

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HCSR04_SPEED_OF_SOUND_CM_US  0.0343f
#define HCSR04_MAX_DISTANCE_CM       400
#define HCSR04_TIMEOUT_US            25000

struct hcsr04_device
{
  rt_base_t trig_pin;
  rt_base_t echo_pin;
};

int hcsr04_init(struct hcsr04_device *dev, rt_base_t trig_pin,
                rt_base_t echo_pin);
int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm);

/* Pure decoder — host-testable. Mirrors logic_tester.reference_decoders.
 * status_code: 0=ok, 1=timeout (echo_us==0), 2=overrange (>=25000),
 *              3=below_min (<116us = 2cm), 4=above_max (>23200us = 400cm).
 * Always writes both cm_x10 and status_code. Returns 0. */
#define HCSR04_STATUS_OK         0
#define HCSR04_STATUS_TIMEOUT    1
#define HCSR04_STATUS_OVERRANGE  2
#define HCSR04_STATUS_BELOW_MIN  3
#define HCSR04_STATUS_ABOVE_MAX  4
int hcsr04_decode_distance(unsigned long echo_us,
                           int *cm_x10,
                           int *status_code);

#ifdef __cplusplus
}
#endif

#endif /* __HCSR04_REF_H */
