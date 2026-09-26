#include "hcsr04.h"
#include "gpio_if.h"
#include "osal_time.h"
#include <stdint.h>

#define HCSR04_TRIG_PULSE_US 10
#define HCSR04_TIMEOUT_US 100000
#define HCSR04_SPEED_OF_SOUND_NUM 343
#define HCSR04_SPEED_OF_SOUND_DEN 2000

int32_t hcsr04_init(struct hcsr04_device *dev, uint16_t trig_pin, uint16_t echo_pin)
{
    int32_t ret;
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;
    ret = GpioSetDir(trig_pin, 1); /* output */
    if (ret != 0) {
        return ret;
    }
    ret = GpioSetDir(echo_pin, 0); /* input */
    if (ret != 0) {
        return ret;
    }
    return 0;
}

static int32_t hcsr04_measure_pulse_us(struct hcsr04_device *dev, uint32_t *duration_us)
{
    uint16_t val;
    uint32_t timeout;
    int32_t ret;
    uint32_t start, end;

    /* Wait for echo pin to go high */
    timeout = 0;
    while (timeout < HCSR04_TIMEOUT_US) {
        ret = GpioRead(dev->echo_pin, &val);
        if (ret != 0) return ret;
        if (val == 1) break;
        OsalUDelay(1);
        timeout++;
    }
    if (timeout >= HCSR04_TIMEOUT_US) {
        return -1; /* timeout */
    }

    /* Measure high pulse width */
    start = 0;
    while (1) {
        ret = GpioRead(dev->echo_pin, &val);
        if (ret != 0) return ret;
        if (val == 0) break;
        start++;
        OsalUDelay(1);
    }

    *duration_us = start;
    return 0;
}

int32_t hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw)
{
    int32_t ret;
    uint32_t pulse_us;

    /* Send trigger pulse */
    ret = GpioWrite(dev->trig_pin, 1);
    if (ret != 0) return ret;
    OsalUDelay(HCSR04_TRIG_PULSE_US);
    ret = GpioWrite(dev->trig_pin, 0);
    if (ret != 0) return ret;

    /* Measure echo pulse */
    ret = hcsr04_measure_pulse_us(dev, &pulse_us);
    if (ret != 0) {
        return ret;
    }

    /* Convert to mm: (pulse_us * 343) / 2000 */
    *raw = (int32_t)(((uint64_t)pulse_us * HCSR04_SPEED_OF_SOUND_NUM) / HCSR04_SPEED_OF_SOUND_DEN);
    return 0;
}