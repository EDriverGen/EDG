#include "hcsr04.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

#define TRIG_PORT GPIOA
#define TRIG_PIN GPIO_PIN_0
#define ECHO_PORT GPIOA
#define ECHO_PIN GPIO_PIN_1

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 8; j++) {}
    }
}

int hcsr04_init(struct hcsr04_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
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

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw) {
    if (!dev || !raw) return -1;

    HAL_GPIO_WritePin((GPIO_TypeDef *)(uintptr_t)dev->trig_port, dev->trig_pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin((GPIO_TypeDef *)(uintptr_t)dev->trig_port, dev->trig_pin, GPIO_PIN_RESET);

    uint32_t timeout = 1000000;
    while (HAL_GPIO_ReadPin((GPIO_TypeDef *)(uintptr_t)dev->echo_port, dev->echo_pin) == GPIO_PIN_RESET) {
        if (--timeout == 0) return -5;
    }

    uint32_t start = 0;
    while (HAL_GPIO_ReadPin((GPIO_TypeDef *)(uintptr_t)dev->echo_port, dev->echo_pin) == GPIO_PIN_SET) {
        start++;
        if (start > 1000000) return -5;
    }

    uint32_t T = start;
    *raw = (int32_t)((T * 343) / 2000);
    return 0;
}