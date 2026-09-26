#include "hcsr04_ref.h"

int hcsr04_init(struct hcsr04_device *dev, GPIO_TypeDef *trig_port, uint16_t trig_pin,
                GPIO_TypeDef *echo_port, uint16_t echo_pin) {
    if (!dev || !trig_port || !echo_port) return -1;
    dev->trig_port = trig_port; dev->trig_pin = trig_pin;
    dev->echo_port = echo_port; dev->echo_pin = echo_pin;
    HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_RESET);
    return 0;
}

int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm) {
    uint32_t start, now, timeout;
    if (!dev || !distance_mm) return -1;
    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_SET);
    hcsr04_delay_us(10);
    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_RESET);
    timeout = HCSR04_TIMEOUT_US;
    while (HAL_GPIO_ReadPin(dev->echo_port, dev->echo_pin) == GPIO_PIN_RESET && timeout > 0)
        { hcsr04_delay_us(1); timeout--; }
    if (timeout == 0) return -1;
    start = hcsr04_get_us_tick();
    while (HAL_GPIO_ReadPin(dev->echo_port, dev->echo_pin) == GPIO_PIN_SET) {
        now = hcsr04_get_us_tick();
        if ((now - start) > HCSR04_TIMEOUT_US) return -1;
    }
    now = hcsr04_get_us_tick();
    *distance_mm = (int32_t)((now - start) * HCSR04_SPEED_OF_SOUND_CM_US * 10.0f / 2.0f);
    return (*distance_mm <= HCSR04_MAX_DISTANCE_CM * 10) ? 0 : -1;
}
