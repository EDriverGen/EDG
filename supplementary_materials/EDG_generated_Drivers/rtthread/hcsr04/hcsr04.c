#include "hcsr04.h"
#include <rtdevice.h>
#include <stdint.h>

#define SPEED_OF_SOUND_MM_PER_US 343
#define TRIGGER_PULSE_US 10
#define MIN_MEASUREMENT_INTERVAL_MS 200

int hcsr04_init(struct hcsr04_device *dev, uint64_t trig_pin, uint64_t echo_pin)
{
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;
    rt_pin_mode(trig_pin, PIN_MODE_OUTPUT);
    rt_pin_mode(echo_pin, PIN_MODE_INPUT);
    rt_pin_write(trig_pin, PIN_LOW);
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *distance_mm)
{
    uint32_t pulse_width_us;
    int32_t distance;

    rt_pin_write(dev->trig_pin, PIN_HIGH);
    rt_hw_us_delay(TRIGGER_PULSE_US);
    rt_pin_write(dev->trig_pin, PIN_LOW);

    pulse_width_us = 0;
    while (rt_pin_read(dev->echo_pin) == PIN_LOW) {
        rt_hw_us_delay(1);
        pulse_width_us++;
        if (pulse_width_us > 100000) {
            return -5; /* -EIO */
        }
    }
    pulse_width_us = 0;
    while (rt_pin_read(dev->echo_pin) == PIN_HIGH) {
        rt_hw_us_delay(1);
        pulse_width_us++;
        if (pulse_width_us > 100000) {
            return -5; /* -EIO */
        }
    }

    distance = (int32_t)(((uint64_t)pulse_width_us * SPEED_OF_SOUND_MM_PER_US) / 2000);
    *distance_mm = distance;
    return 0;
}
