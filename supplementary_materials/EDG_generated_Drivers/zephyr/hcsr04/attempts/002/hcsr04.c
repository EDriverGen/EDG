#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
#include <stdint.h>

#define TRIG_NODE DT_NODELABEL(hcsr04_trig)
#define ECHO_NODE DT_NODELABEL(hcsr04_echo)

static const struct gpio_dt_spec trig_spec = GPIO_DT_SPEC_GET(TRIG_NODE, gpios);
static const struct gpio_dt_spec echo_spec = GPIO_DT_SPEC_GET(ECHO_NODE, gpios);

int hcsr04_init(const struct device *dev)
{
    int ret;

    ret = gpio_pin_configure_dt(&trig_spec, GPIO_OUTPUT_INIT_LOW);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_pin_configure_dt(&echo_spec, GPIO_INPUT);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int hcsr04_read_distance(const struct device *dev, int32_t *raw)
{
    int ret;
    uint32_t pulse_width_us = 0;
    int pin_state;
    int64_t distance;

    /* Trigger pulse: 10 us high */
    ret = gpio_pin_set_dt(&trig_spec, 1);
    if (ret < 0) {
        return ret;
    }
    k_busy_wait(10);
    ret = gpio_pin_set_dt(&trig_spec, 0);
    if (ret < 0) {
        return ret;
    }

    /* Wait for echo start (pin goes high) */
    /* Timeout: 200 ms */
    uint32_t timeout = 200000; /* in microseconds */
    while (timeout > 0) {
        pin_state = gpio_pin_get_dt(&echo_spec);
        if (pin_state < 0) {
            return pin_state;
        }
        if (pin_state == 1) {
            break;
        }
        k_busy_wait(1);
        timeout--;
    }
    if (timeout == 0) {
        return -EIO;
    }

    /* Measure echo pulse width */
    timeout = 200000;
    while (timeout > 0) {
        pin_state = gpio_pin_get_dt(&echo_spec);
        if (pin_state < 0) {
            return pin_state;
        }
        if (pin_state == 0) {
            break;
        }
        k_busy_wait(1);
        pulse_width_us++;
        timeout--;
    }
    if (timeout == 0) {
        return -EIO;
    }

    /* Convert pulse width to distance in mm: (T * 343) // 2000 */
    distance = (int64_t)pulse_width_us * 343 / 2000;
    *raw = (int32_t)distance;

    return 0;
}