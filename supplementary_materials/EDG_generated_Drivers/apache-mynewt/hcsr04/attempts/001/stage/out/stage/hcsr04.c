#include "hcsr04.h"
#include <errno.h>

#include <hal/hal_gpio.h>
#include "apache_mynewt.h"
#define TIMEOUT_US 100000

int hcsr04_init(struct hcsr04_dev *dev, int trig_pin, int echo_pin) {
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;
    hal_gpio_init_out(trig_pin, 0);
    hal_gpio_init_in(echo_pin, HAL_GPIO_PULL_NONE);
    return 0;
}

int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw) {
    int trig = dev->trig_pin;
    int echo = dev->echo_pin;
    int timeout = TIMEOUT_US;
    int pulse_width = 0;

    // Trigger pulse: 10 us high
    hal_gpio_write(trig, 1);
    os_cputime_delay_usecs(10);
    hal_gpio_write(trig, 0);

    // Wait for echo pin to go high
    while (hal_gpio_read(echo) == 0) {
        if (--timeout <= 0) {
            return -EIO;
        }
        os_cputime_delay_usecs(1);
    }

    // Measure high pulse width
    timeout = TIMEOUT_US;
    while (hal_gpio_read(echo) == 1) {
        if (--timeout <= 0) {
            return -EIO;
        }
        os_cputime_delay_usecs(1);
        pulse_width++;
    }

    // Convert to mm: (pulse_width * 343) / 2000
    *raw = (int32_t)(((int64_t)pulse_width * 343) / 2000);
    return 0;
}
