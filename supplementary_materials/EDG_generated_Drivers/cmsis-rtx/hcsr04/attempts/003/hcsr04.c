#include "hcsr04.h"
#include <stdint.h>
#include <stddef.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"

#define TRIG_PULSE_US 10
#define TIMEOUT_US 60000
#define SOUND_SPEED_NUM 343
#define SOUND_SPEED_DEN 2000

static uint32_t measure_pulse_us(GPIO_TypeDef *port, uint16_t pin) {
    uint32_t count = 0;
    uint32_t timeout = TIMEOUT_US;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        if (--timeout == 0) return 0;
        HAL_Delay(0);
    }
    timeout = TIMEOUT_US;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        if (--timeout == 0) return 0;
        count++;
        HAL_Delay(0);
    }
    return count;
}

int hcsr04_init(struct hcsr04_dev *dev, GPIO_TypeDef *trig_port, uint16_t trig_pin, GPIO_TypeDef *echo_port, uint16_t echo_pin) {
    if (!dev || !trig_port || !echo_port) return -1;
    dev->trig_port = trig_port;
    dev->trig_pin = trig_pin;
    dev->echo_port = echo_port;
    dev->echo_pin = echo_pin;

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = trig_pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(trig_port, &gpio_init);

    gpio_init.Pin = echo_pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(echo_port, &gpio_init);

    HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_RESET);
    return 0;
}

int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw) {
    if (!dev || !raw) return -1;

    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_SET);
    HAL_Delay(0);
    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_RESET);

    uint32_t pulse_us = measure_pulse_us(dev->echo_port, dev->echo_pin);
    if (pulse_us == 0) return -5;

    *raw = (int32_t)((pulse_us * SOUND_SPEED_NUM) / SOUND_SPEED_DEN);
    return 0;
}