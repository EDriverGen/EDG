#include "hcsr04.h"
#include "gpio_if.h"
#include "osal_time.h"
#include <stdint.h>

#define TRIG_PULSE_US 10
#define TIMEOUT_US 100000
#define SOUND_SPEED_NUM 343
#define SOUND_SPEED_DEN 2000

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

static int32_t measure_pulse_us(uint16_t gpio, uint32_t *duration_us)
{
    uint16_t val;
    uint32_t timeout = 0;
    /* Wait for echo pin to go high */
    while (1) {
        GpioRead(gpio, &val);
        if (val == 1) break;
        if (++timeout > TIMEOUT_US / 10) return -1; /* timeout */
        OsalUDelay(10);
    }
    /* Measure high pulse width */
    uint32_t count = 0;
    while (1) {
        GpioRead(gpio, &val);
        if (val == 0) break;
        count++;
        if (count > TIMEOUT_US) return -1; /* timeout */
        OsalUDelay(1);
    }
    *duration_us = count;
    return 0;
}

int32_t hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw)
{
    int32_t ret;
    uint32_t pulse_us;
    /* Trigger pulse */
    GpioWrite(dev->trig_pin, 1);
    OsalUDelay(TRIG_PULSE_US);
    GpioWrite(dev->trig_pin, 0);
    /* Measure echo */
    ret = measure_pulse_us(dev->echo_pin, &pulse_us);
    if (ret != 0) return -1; /* -EIO */
    /* Convert to mm: (pulse_us * 343) / 2000 */
    *raw = (int32_t)(((uint64_t)pulse_us * SOUND_SPEED_NUM) / SOUND_SPEED_DEN);
    return 0;
}