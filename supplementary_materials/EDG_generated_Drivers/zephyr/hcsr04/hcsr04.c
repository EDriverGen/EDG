#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
#include <stdint.h>

#define TRIG_PULSE_US 10
#define TIMEOUT_US 100000

static const struct gpio_dt_spec trig_spec = GPIO_DT_SPEC_GET(DT_NODELABEL(hcsr04_trig), gpios);
static const struct gpio_dt_spec echo_spec = GPIO_DT_SPEC_GET(DT_NODELABEL(hcsr04_echo), gpios);

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
    int64_t distance;

    /* Trigger pulse */
    ret = gpio_pin_set_dt(&trig_spec, 1);
    if (ret < 0) {
        return ret;
    }
    k_busy_wait(TRIG_PULSE_US);
    ret = gpio_pin_set_dt(&trig_spec, 0);
    if (ret < 0) {
        return ret;
    }

    /* Wait for echo start (rising edge) with timeout */
    uint32_t timeout = TIMEOUT_US;
    while (gpio_pin_get_dt(&echo_spec) == 0) {
        k_busy_wait(1);
        if (--timeout == 0) {
            return -EIO;
        }
    }

    /* Measure pulse width */
    timeout = TIMEOUT_US;
    while (gpio_pin_get_dt(&echo_spec) == 1) {
        k_busy_wait(1);
        pulse_width_us++;
        if (--timeout == 0) {
            return -EIO;
        }
    }

    /* Convert to mm: (pulse_width_us * 343) // 2000 */
    distance = (int64_t)pulse_width_us * 343;
    distance = distance / 2000;

    *raw = (int32_t)distance;
    return 0;
}