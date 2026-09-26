#include "hcsr04.h"
#include <errno.h>
#include <stdint.h>

#include <hal/hal_gpio.h>
#include "apache_mynewt.h"
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
    int timeout = 100000;
    int t;

    hal_gpio_write(trig, 1);
    os_cputime_delay_usecs(10);
    hal_gpio_write(trig, 0);

    while (hal_gpio_read(echo) == 0) {
        if (--timeout <= 0) return -EIO;
    }
    t = 0;
    while (hal_gpio_read(echo) == 1) {
        if (--timeout <= 0) return -EIO;
        t++;
    }
    *raw = (t * 343) / 2000;
    return 0;
}
