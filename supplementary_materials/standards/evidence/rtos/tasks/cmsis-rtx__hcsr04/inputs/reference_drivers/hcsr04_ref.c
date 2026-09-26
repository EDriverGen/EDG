#include "hcsr04_ref.h"

static void hcsr04_delay_us(uint32_t us)
{
    (void)us;
}

int hcsr04_init(struct hcsr04_device *dev, GPIO_TypeDef *trig_port, uint16_t trig_pin,
                GPIO_TypeDef *echo_port, uint16_t echo_pin)
{
    GPIO_InitTypeDef gpio;
    if (dev == 0 || trig_port == 0 || echo_port == 0) {
        return -1;
    }
    dev->trig_port = trig_port;
    dev->trig_pin = trig_pin;
    dev->echo_port = echo_port;
    dev->echo_pin = echo_pin;

    gpio.Pin = trig_pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(trig_port, &gpio);
    gpio.Pin = echo_pin;
    gpio.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(echo_port, &gpio);
    HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_RESET);
    return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm)
{
    uint32_t timeout;
    uint32_t elapsed;
    if (dev == 0 || distance_mm == 0) {
        return -1;
    }
    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_SET);
    hcsr04_delay_us(10);
    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_RESET);

    timeout = HCSR04_TIMEOUT_US;
    while (HAL_GPIO_ReadPin(dev->echo_port, dev->echo_pin) == GPIO_PIN_RESET && timeout > 0) {
        hcsr04_delay_us(1);
        timeout--;
    }
    if (timeout == 0) {
        return -1;
    }
    timeout = HCSR04_TIMEOUT_US;
    while (HAL_GPIO_ReadPin(dev->echo_port, dev->echo_pin) == GPIO_PIN_SET && timeout > 0) {
        hcsr04_delay_us(1);
        timeout--;
    }
    if (timeout == 0) {
        return -1;
    }
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
    if (echo_us == 0) {
        status = HCSR04_STATUS_TIMEOUT;
    } else if (echo_us >= overrange_us) {
        status = HCSR04_STATUS_OVERRANGE;
    } else {
        cm = (int)((echo_us * 10UL) / 58UL);
        if (echo_us < min_us) status = HCSR04_STATUS_BELOW_MIN;
        else if (echo_us > max_us) status = HCSR04_STATUS_ABOVE_MAX;
        else status = HCSR04_STATUS_OK;
    }
    if (cm_x10) *cm_x10 = cm;
    if (status_code) *status_code = status;
    return 0;
}
