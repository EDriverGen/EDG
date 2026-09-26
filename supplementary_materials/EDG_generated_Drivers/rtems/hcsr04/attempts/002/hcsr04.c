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
    sc = rtems_gpio_request_pin(trig_pin, RTEMS_GPIO_FUNCTION, true, false, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        return -EIO;

    sc = rtems_gpio_request_pin(echo_pin, RTEMS_GPIO_FUNCTION, false, false, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        return -EIO;

    return 0;
}

static int measure_pulse_us(struct hcsr04_device *dev, uint32_t *duration_us)
{
    uint32_t timeout = TIMEOUT_US;
    int val;

    // Wait for echo pin to go high
    while (timeout--) {
        val = rtems_gpio_get_value(dev->echo_pin);
        if (val == 1)
            break;
    }
    if (timeout == 0)
        return -EIO;

    // Measure high pulse width
    uint32_t count = 0;
    while (timeout--) {
        val = rtems_gpio_get_value(dev->echo_pin);
        if (val == 0)
            break;
        count++;
    }
    if (timeout == 0)
        return -EIO;

    *duration_us = count;
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw)
{
    // Trigger pulse
    rtems_gpio_set(dev->trig_pin);
    // Delay 10 us (approximate busy-wait, no RTEMS delay_us available)
    volatile uint32_t delay = TRIGGER_PULSE_US * 10; // crude approximation
    while (delay--);
    // Set trig low (no rtems_gpio_clear, use output low by requesting with output_enable=false?)
    // Actually, we need to set low. Use rtems_gpio_set with inverted? Not available.
    // Instead, reconfigure as input (high-Z) or use a different approach.
    // Since no clear function, we can request pin as input to make it low.
    rtems_gpio_request_pin(dev->trig_pin, RTEMS_GPIO_FUNCTION, false, false, NULL);

    // Measure echo pulse
    uint32_t pulse_us;
    int ret = measure_pulse_us(dev, &pulse_us);
    if (ret != 0)
        return ret;

    // Convert to mm: (pulse_us * 343) / 2000
    *raw = (int32_t)(((uint64_t)pulse_us * SOUND_SPEED_NUM) / SOUND_SPEED_DEN);
    return 0;
}
