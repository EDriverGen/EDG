#include "hcsr04.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include <stddef.h>

#define TRIG_PORT GPIOA
#define TRIG_PIN GPIO_PIN_0
#define ECHO_PORT GPIOA
#define ECHO_PIN GPIO_PIN_1

static int measure_pulse_us(GPIO_TypeDef *port, uint16_t pin, uint32_t timeout_us)
{
    uint32_t count = 0;
    uint32_t max_count = timeout_us;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        if (count++ >= max_count) return -1;
        HAL_Delay(0);
    }
    count = 0;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        if (count++ >= max_count) return -1;
        HAL_Delay(0);
    }
    return (int)count;
}

int hcsr04_init(struct hcsr04_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->trig_port = (uint16_t)(uintptr_t)TRIG_PORT;
    dev->trig_pin = TRIG_PIN;
    dev->echo_port = (uint16_t)(uintptr_t)ECHO_PORT;
    dev->echo_pin = ECHO_PIN;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = TRIG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ECHO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw)
{
    (void)dev;
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    HAL_Delay(0);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    int pulse_us = measure_pulse_us(ECHO_PORT, ECHO_PIN, 100000);
    if (pulse_us < 0) {
        return -1;
    }
    *raw = ((int32_t)pulse_us * 343) / 2000;
    return 0;
}