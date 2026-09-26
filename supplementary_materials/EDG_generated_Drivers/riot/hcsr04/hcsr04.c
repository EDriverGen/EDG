#include "hcsr04.h"
#include <errno.h>
#include <stdint.h>
#include "xtimer.h"

#define TRIGGER_PULSE_US 10
#define TIMEOUT_US 100000
#define SOUND_SPEED_NUM 343
#define SOUND_SPEED_DEN 2000

int hcsr04_init(hcsr04_t *dev, gpio_t trig, gpio_t echo)
{
    dev->trig_pin = trig;
    dev->echo_pin = echo;
    gpio_init(trig, GPIO_OUT);
    gpio_init(echo, GPIO_IN);
    gpio_clear(trig);
    return 0;
}

int hcsr04_read_distance(hcsr04_t *dev, int32_t *raw)
{
    uint32_t pulse_width = 0;
    uint32_t timeout;
    int ret;

    gpio_set(dev->trig_pin);
    xtimer_usleep(TRIGGER_PULSE_US);
    gpio_clear(dev->trig_pin);

    timeout = 0;
    while (gpio_read(dev->echo_pin) == 0) {
        xtimer_usleep(1);
        timeout++;
        if (timeout > TIMEOUT_US) {
            return -EIO;
        }
    }

    timeout = 0;
    while (gpio_read(dev->echo_pin) == 1) {
        xtimer_usleep(1);
        pulse_width++;
        timeout++;
        if (timeout > TIMEOUT_US) {
            return -EIO;
        }
    }

    *raw = (int32_t)((pulse_width * SOUND_SPEED_NUM) / SOUND_SPEED_DEN);
    return 0;
}