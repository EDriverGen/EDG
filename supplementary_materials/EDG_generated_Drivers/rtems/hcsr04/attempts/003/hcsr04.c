#include "hcsr04.h"
#include <errno.h>
#include <stdint.h>

#include <rtems/gpio.h>
#include "rtems.h"
#define TRIGGER_PULSE_US 10
#define TIMEOUT_US 100000
#define SPEED_OF_SOUND_MM_PER_US 343

static void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 10; i++) {
        __asm__ volatile ("nop");
    }
}

int hcsr04_init(struct hcsr04_device *dev, uint32_t trig_pin, uint32_t echo_pin) {
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;

    rtems_status_code sc;
    sc = rtems_gpio_request_pin(trig_pin, RTEMS_GPIO_OUTPUT, true, false, NULL);
    if (sc != RTEMS_SUCCESSFUL) {
        return -EIO;
    }
    sc = rtems_gpio_request_pin(echo_pin, RTEMS_GPIO_INPUT, false, false, NULL);
    if (sc != RTEMS_SUCCESSFUL) {
        return -EIO;
    }
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw) {
    uint32_t trig = dev->trig_pin;
    uint32_t echo = dev->echo_pin;

    rtems_gpio_set(trig);
    delay_us(TRIGGER_PULSE_US);
    rtems_gpio_clear(trig);

    uint32_t timeout = 0;
    while (rtems_gpio_get_value(echo) == 0) {
        delay_us(1);
        timeout++;
        if (timeout > TIMEOUT_US) {
            return -EIO;
        }
    }

    uint32_t pulse_start = 0;
    while (rtems_gpio_get_value(echo) == 1) {
        delay_us(1);
        pulse_start++;
        if (pulse_start > TIMEOUT_US) {
            return -EIO;
        }
    }

    uint32_t pulse_width_us = pulse_start;
    *raw = (int32_t)((pulse_width_us * SPEED_OF_SOUND_MM_PER_US) / 2000);
    return 0;
}
