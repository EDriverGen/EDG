#include "hcsr04_ref.h"

static uint64_t hcsr04_get_time_us(void)
{
    OsalTimespec ts;

    if (OsalGetTime(&ts) != HDF_SUCCESS) {
        return 0;
    }
    return ts.sec * 1000000ULL + ts.usec;
}

int hcsr04_init(struct hcsr04_device *dev, uint16_t trig_gpio, uint16_t echo_gpio)
{
    if (dev == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    dev->trig_gpio = trig_gpio;
    dev->echo_gpio = echo_gpio;

    if (GpioSetDir(trig_gpio, GPIO_DIR_OUT) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    if (GpioSetDir(echo_gpio, GPIO_DIR_IN) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    if (GpioWrite(trig_gpio, GPIO_VAL_LOW) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
    uint16_t gpio_val = GPIO_VAL_LOW;
    uint64_t start_us;
    uint64_t now_us;

    if (dev == NULL || distance_mm == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    GpioWrite(dev->trig_gpio, GPIO_VAL_LOW);
    OsalUDelay(2);
    GpioWrite(dev->trig_gpio, GPIO_VAL_HIGH);
    OsalUDelay(10);
    GpioWrite(dev->trig_gpio, GPIO_VAL_LOW);

    start_us = hcsr04_get_time_us();
    do {
        if (GpioRead(dev->echo_gpio, &gpio_val) != HDF_SUCCESS) {
            return HDF_FAILURE;
        }
        now_us = hcsr04_get_time_us();
        if ((now_us - start_us) > HCSR04_TIMEOUT_US) {
            return HDF_ERR_TIMEOUT;
        }
    } while (gpio_val == GPIO_VAL_LOW);

    start_us = now_us;
    do {
        if (GpioRead(dev->echo_gpio, &gpio_val) != HDF_SUCCESS) {
            return HDF_FAILURE;
        }
        now_us = hcsr04_get_time_us();
        if ((now_us - start_us) > HCSR04_TIMEOUT_US) {
            return HDF_ERR_TIMEOUT;
        }
    } while (gpio_val == GPIO_VAL_HIGH);

    *distance_mm = (int32_t)(((double)(now_us - start_us) * HCSR04_SPEED_OF_SOUND_CM_US * 10.0) / 2.0);
    return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? HDF_SUCCESS : HDF_ERR_OUT_OF_RANGE;
}
