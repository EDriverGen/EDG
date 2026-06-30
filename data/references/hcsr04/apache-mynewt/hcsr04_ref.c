#include "hcsr04_ref.h"

int hcsr04_init(struct hcsr04_device *dev, int trig_pin, int echo_pin)
{
    if (dev == 0) {
        return -1;
    }
    dev->trig_pin = trig_pin;
    dev->echo_pin = echo_pin;
    if (hal_gpio_init_out(trig_pin, 0) != 0 || hal_gpio_init_in(echo_pin, 0) != 0) {
        return -1;
    }
    return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
    uint32_t timeout;
    uint32_t elapsed;
    if (dev == 0 || distance_mm == 0) {
        return -1;
    }
    hal_gpio_write(dev->trig_pin, 1);
    os_cputime_delay_usecs(10);
    hal_gpio_write(dev->trig_pin, 0);

    timeout = HCSR04_TIMEOUT_US;
    while (hal_gpio_read(dev->echo_pin) == 0 && timeout > 0) {
        os_cputime_delay_usecs(1);
        timeout--;
    }
    if (timeout == 0) return -1;
    timeout = HCSR04_TIMEOUT_US;
    while (hal_gpio_read(dev->echo_pin) != 0 && timeout > 0) {
        os_cputime_delay_usecs(1);
        timeout--;
    }
    if (timeout == 0) return -1;
    elapsed = HCSR04_TIMEOUT_US - timeout;
    *distance_mm = (int32_t)(((elapsed * 10U) + 29U) / 58U);
    return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}

int hcsr04_decode_distance(unsigned long echo_us, int *cm_x10, int *status_code)
{
    const unsigned long min_us = 116;
    const unsigned long max_us = 23200;
    const unsigned long overrange_us = 25000;
    int status;
    int cm = 0;
    if (echo_us == 0) status = HCSR04_STATUS_TIMEOUT;
    else if (echo_us >= overrange_us) status = HCSR04_STATUS_OVERRANGE;
    else {
        cm = (int)((echo_us * 10UL) / 58UL);
        if (echo_us < min_us) status = HCSR04_STATUS_BELOW_MIN;
        else if (echo_us > max_us) status = HCSR04_STATUS_ABOVE_MAX;
        else status = HCSR04_STATUS_OK;
    }
    if (cm_x10) *cm_x10 = cm;
    if (status_code) *status_code = status;
    return 0;
}
