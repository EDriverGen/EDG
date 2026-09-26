#include "hcsr04.h"
#include "gpio_if.h"
#include "osal_time.h"
#include <stdint.h>

#define HCSR04_TRIG_PULSE_US 10
#define HCSR04_TIMEOUT_US 100000
#define HCSR04_SOUND_SPEED_NUM 343
#define HCSR04_SOUND_SPEED_DEN 2000

int32_t hcsr04_init(struct hcsr04_device *dev, uint16_t trig_pin, uint16_t echo_pin)
{
    int32_t ret;
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;
    ret = GpioSetDir(trig_pin, 1); /* output */
    if (ret != 0) return ret;
    ret = GpioSetDir(echo_pin, 0); /* input */
    if (ret != 0) return ret;
    return 0;
}

static int32_t hcsr04_measure_pulse_us(struct hcsr04_device *dev, uint32_t *duration_us)
{
    uint16_t val;
    uint32_t timeout;
    int32_t ret;
    /* Wait for echo pin to go high */
    timeout = HCSR04_TIMEOUT_US;
    while (timeout > 0) {
        ret = GpioRead(dev->echo_pin, &val);
        if (ret != 0) return ret;
        if (val == 1) break;
        OsalUDelay(1);
        timeout--;
    }
    if (timeout == 0) return -1; /* timeout */
    /* Measure high pulse width */
    *duration_us = 0;
    timeout = HCSR04_TIMEOUT_US;
    while (timeout > 0) {
        ret = GpioRead(dev->echo_pin, &val);
        if (ret != 0) return ret;
        if (val == 0) break;
        (*duration_us)++;
        OsalUDelay(1);
        timeout--;
    }
    if (timeout == 0) return -1; /* timeout */
    return 0;
}

int32_t hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw)
{
    int32_t ret;
    uint32_t pulse_us;
    /* Trigger pulse */
    ret = GpioWrite(dev->trig_pin, 1);
    if (ret != 0) return ret;
    OsalUDelay(HCSR04_TRIG_PULSE_US);
    ret = GpioWrite(dev->trig_pin, 0);
    if (ret != 0) return ret;
    /* Measure echo pulse */
    ret = hcsr04_measure_pulse_us(dev, &pulse_us);
    if (ret != 0) return -1; /* echo timeout */
    /* Convert to mm: (pulse_us * 343) / 2000 */
    *raw = (int32_t)(((uint64_t)pulse_us * HCSR04_SOUND_SPEED_NUM) / HCSR04_SOUND_SPEED_DEN);
    return 0;
}