/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for RT-Thread
 */
#include "hcsr04_ref.h"

int hcsr04_init(struct hcsr04_device *dev, rt_base_t trig_pin,
                rt_base_t echo_pin)
{
  if (dev == NULL) return -1;

  dev->trig_pin = trig_pin;
  dev->echo_pin = echo_pin;

  rt_pin_mode(trig_pin, PIN_MODE_OUTPUT);
  rt_pin_mode(echo_pin, PIN_MODE_INPUT);
  rt_pin_write(trig_pin, PIN_LOW);

  return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
  rt_tick_t start, elapsed;
  uint32_t timeout;

  if (dev == NULL || distance_mm == NULL) return -1;

  /* Send 10us trigger pulse */
  rt_pin_write(dev->trig_pin, PIN_HIGH);
  rt_hw_us_delay(10);
  rt_pin_write(dev->trig_pin, PIN_LOW);

  /* Wait for echo HIGH */
  timeout = HCSR04_TIMEOUT_US;
  while (rt_pin_read(dev->echo_pin) == PIN_LOW && timeout > 0)
    { rt_hw_us_delay(1); timeout--; }
  if (timeout == 0) return -1;

  start = rt_tick_get();

  /* Wait for echo LOW */
  timeout = HCSR04_TIMEOUT_US;
  while (rt_pin_read(dev->echo_pin) == PIN_HIGH && timeout > 0)
    { rt_hw_us_delay(1); timeout--; }
  if (timeout == 0) return -1;

  elapsed = HCSR04_TIMEOUT_US - timeout;
  *distance_mm = (int32_t)(elapsed * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);

  return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}

/* Pure-logic distance decoder shared with host-side logic tests. Uses
 * integer math (cm_x10 = echo_us * 10 / 58) to match the reference
 * Python decoder bit-for-bit. */
int hcsr04_decode_distance(unsigned long echo_us,
                           int *cm_x10,
                           int *status_code)
{
    /* Mirror reference_decoders.HCSR04_*_US thresholds. */
    const unsigned long min_us = 116;       /* 2 cm round-trip */
    const unsigned long max_us = 23200;     /* 400 cm round-trip */
    const unsigned long overrange_us = 25000;

    int status;
    int cm = 0;

    if (echo_us == 0) {
        status = HCSR04_STATUS_TIMEOUT;
    } else if (echo_us >= overrange_us) {
        status = HCSR04_STATUS_OVERRANGE;
    } else {
        cm = (int)((echo_us * 10UL) / 58UL);
        if (echo_us < min_us)        status = HCSR04_STATUS_BELOW_MIN;
        else if (echo_us > max_us)   status = HCSR04_STATUS_ABOVE_MAX;
        else                         status = HCSR04_STATUS_OK;
    }
    if (cm_x10)       *cm_x10 = cm;
    if (status_code)  *status_code = status;
    return 0;
}
