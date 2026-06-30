/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * HC-SR04 Ultrasonic Distance Sensor Driver for ChibiOS
 */
#include "hcsr04_ref.h"

int hcsr04_init(struct hcsr04_device *dev,
                ioportid_t trig_port, ioportmask_t trig_pad,
                ioportid_t echo_port, ioportmask_t echo_pad)
{
  if (dev == NULL) return -1;

  dev->trig_port = trig_port;
  dev->trig_pad = trig_pad;
  dev->echo_port = echo_port;
  dev->echo_pad = echo_pad;

  palSetPadMode(trig_port, trig_pad, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(echo_port, echo_pad, PAL_MODE_INPUT);
  palClearPad(trig_port, trig_pad);

  return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
  rtcnt_t start, end;
  uint32_t timeout;
  uint32_t elapsed_us;

  if (dev == NULL || distance_mm == NULL) return -1;

  palSetPad(dev->trig_port, dev->trig_pad);
  chSysPolledDelayX(US2RTC(STM32_HCLK, 10));
  palClearPad(dev->trig_port, dev->trig_pad);

  timeout = HCSR04_TIMEOUT_US;
  while (palReadPad(dev->echo_port, dev->echo_pad) == PAL_LOW && timeout > 0)
    { chSysPolledDelayX(US2RTC(STM32_HCLK, 1)); timeout--; }
  if (timeout == 0) return -1;

  start = chSysGetRealtimeCounterX();
  while (palReadPad(dev->echo_port, dev->echo_pad) == PAL_HIGH)
    {
      end = chSysGetRealtimeCounterX();
      elapsed_us = RTC2US(STM32_HCLK, end - start);
      if (elapsed_us > HCSR04_TIMEOUT_US) return -1;
    }
  end = chSysGetRealtimeCounterX();
  elapsed_us = RTC2US(STM32_HCLK, end - start);

  *distance_mm = (int32_t)(elapsed_us * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);
  return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}
