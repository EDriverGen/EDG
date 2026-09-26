#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
#include <stdint.h>

#define TRIG_PIN_NODE DT_NODELABEL(hcsr04_trig)
#define ECHO_PIN_NODE DT_NODELABEL(hcsr04_echo)

static const struct gpio_dt_spec trig_spec = GPIO_DT_SPEC_GET(TRIG_PIN_NODE, gpios);
static const struct gpio_dt_spec echo_spec = GPIO_DT_SPEC_GET(ECHO_PIN_NODE, gpios);

int hcsr04_init(void)
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

int hcsr04_read_distance(int32_t *raw)
{
    int ret;
    uint32_t pulse_width_us = 0;
    int pin_value;
    int64_t distance_mm;

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
    uint32_t timeout_cycles = 200000; /* 200 ms in us, poll every 10 us */
    uint32_t poll_count = 0;
    while (poll_count < timeout_cycles) {
        pin_value = gpio_pin_get_dt(&echo_spec);
        if (pin_value < 0) {
            return pin_value;
        }
        if (pin_value == 1) {
            break;
        }
        k_busy_wait(10);
        poll_count += 10;
    }
    if (poll_count >= timeout_cycles) {
        return -EIO;
    }

    /* Measure pulse width */
    poll_count = 0;
    while (poll_count < timeout_cycles) {
        pin_value = gpio_pin_get_dt(&echo_spec);
        if (pin_value < 0) {
            return pin_value;
        }
        if (pin_value == 0) {
            break;
        }
        k_busy_wait(1);
        pulse_width_us += 1;
        poll_count += 1;
    }
    if (poll_count >= timeout_cycles) {
        return -EIO;
    }

    /* Compute distance: (T * 343) // 2000 */
    distance_mm = ((int64_t)pulse_width_us * 343) / 2000;
    *raw = (int32_t)distance_mm;

    return 0;
}