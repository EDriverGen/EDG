#include "hcsr04.h"
#include <errno.h>
#include <stdint.h>

#include <rtems/gpio.h>
#include "rtems.h"
#define TRIGGER_PULSE_US 10
#define TIMEOUT_US 100000
#define SOUND_SPEED_NUM 343
#define SOUND_SPEED_DEN 2000

int hcsr04_init(struct hcsr04_device *dev, uint32_t trig_pin, uint32_t echo_pin)
{
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;

    rtems_status_code sc;
    sc = rtems_gpio_request_pin(trig_pin, RTEMS_GPIO_PIN_OUTPUT, true, false, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        return -EIO;
    sc = rtems_gpio_request_pin(echo_pin, RTEMS_GPIO_PIN_INPUT, false, false, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        return -EIO;

    return 0;
}

static int measure_pulse_us(uint32_t echo_pin, uint32_t timeout_us)
{
    int val;
    uint32_t count = 0;
    const uint32_t max_count = timeout_us;

    while (count < max_count) {
        val = rtems_gpio_get_value(echo_pin);
        if (val == 1)
            break;
        count++;
    }
    if (count >= max_count)
        return -1;

    count = 0;
    while (count < max_count) {
        val = rtems_gpio_get_value(echo_pin);
        if (val == 0)
            break;
        count++;
    }
    if (count >= max_count)
        return -1;

    return (int)count;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw)
{
    rtems_status_code sc;
    int pulse_width;

    sc = rtems_gpio_set(dev->trig_pin);
    if (sc != RTEMS_SUCCESSFUL)
        return -EIO;

    for (volatile int i = 0; i < TRIGGER_PULSE_US; i++) {}

    sc = rtems_gpio_set(dev->trig_pin);
    if (sc != RTEMS_SUCCESSFUL)
        return -EIO;

    pulse_width = measure_pulse_us(dev->echo_pin, TIMEOUT_US);
    if (pulse_width < 0)
        return -EIO;

    *raw = ((int32_t)pulse_width * SOUND_SPEED_NUM) / SOUND_SPEED_DEN;
    return 0;
}
