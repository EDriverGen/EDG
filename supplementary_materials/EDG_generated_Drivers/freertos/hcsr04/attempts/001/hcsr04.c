#include "hcsr04.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_gpio.h"
#define ECHO_TIMEOUT_US 100000

int hcsr04_init(struct hcsr04_dev *dev, GPIO_TypeDef *port, uint16_t trig, uint16_t echo)
{
    if (!dev || !port) return -1;
    dev->gpio_port = port;
    dev->trig_pin = trig;
    dev->echo_pin = echo;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = trig;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = echo;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(port, trig, GPIO_PIN_RESET);

    return 0;
}

int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw)
{
    if (!dev || !raw) return -1;

    GPIO_TypeDef *port = dev->gpio_port;
    uint16_t trig = dev->trig_pin;
    uint16_t echo = dev->echo_pin;

    HAL_GPIO_WritePin(port, trig, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(port, trig, GPIO_PIN_RESET);

    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(port, echo) == GPIO_PIN_RESET) {
        HAL_Delay(1);
        timeout++;
        if (timeout > 100) return -EIO;
    }

    uint32_t pulse_start = 0;
    uint32_t pulse_end = 0;
    uint32_t count = 0;
    while (HAL_GPIO_ReadPin(port, echo) == GPIO_PIN_SET) {
        HAL_Delay(1);
        count++;
        if (count > 100000) return -EIO;
    }
    pulse_end = count;

    uint32_t T = pulse_end - pulse_start;
    *raw = (int32_t)((T * 343) / 2000);

    return 0;
}
