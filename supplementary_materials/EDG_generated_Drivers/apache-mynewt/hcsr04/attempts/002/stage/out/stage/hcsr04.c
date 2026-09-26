#include "hcsr04.h"
#include <errno.h>
#include <stdint.h>

#include <hal/hal_gpio.h>
#include "apache_mynewt.h"
int hcsr04_init(struct hcsr04_dev *dev, int trig_pin, int echo_pin)
{
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;
    hal_gpio_init_out(trig_pin, 0);
    hal_gpio_init_in(echo_pin, HAL_GPIO_PULL_NONE);
    return 0;
}

int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw)
{
    int timeout;
    int pulse_width = 0;
    int val;

    /* Trigger pulse: 10 us high */
    hal_gpio_write(dev->trig_pin, 1);
    os_cputime_delay_usecs(10);
    hal_gpio_write(dev->trig_pin, 0);

    /* Wait for echo pin to go high (start of pulse) */
    timeout = 10000; /* 10 ms timeout */
    while (hal_gpio_read(dev->echo_pin) == 0) {
        if (--timeout <= 0) {
            return -EIO;
        }
        os_cputime_delay_usecs(1);
    }

    /* Measure high pulse width */
    timeout = 100000; /* 100 ms timeout */
    while (hal_gpio_read(dev->echo_pin) == 1) {
        if (--timeout <= 0) {
            return -EIO;
        }
        os_cputime_delay_usecs(1);
        pulse_width++;
    }

    /* Convert pulse width (us) to distance (mm): (T * 343) / 2000 */
    *raw = (int32_t)(((int64_t)pulse_width * 343) / 2000);

    return 0;
}
