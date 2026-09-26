#include "hcsr04.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <stdint.h>

#define TRIG_PIN GPIO_PIN_0
#define ECHO_PIN GPIO_PIN_1
#define TRIG_PORT GPIOA
#define ECHO_PORT GPIOA

#define TIMEOUT_US 100000

static uint32_t measure_pulse_us(GPIO_TypeDef *port, uint16_t pin, uint32_t timeout_us)
{
    uint32_t count = 0;
    uint32_t max_loops = timeout_us;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        if (++count > max_loops) return 0;
        HAL_Delay(0);
    }
    count = 0;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        if (++count > max_loops) return 0;
        HAL_Delay(0);
    }
    return count;
}

int hcsr04_init(struct hcsr04_dev *dev)
{
    if (!dev) return -1;
    dev->bus_handle = NULL;
    dev->trig_pin = TRIG_PIN;
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

int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw)
{
    if (!dev || !raw) return -1;

    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    HAL_Delay(0);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    uint32_t pulse_width = measure_pulse_us(ECHO_PORT, ECHO_PIN, TIMEOUT_US);
    if (pulse_width == 0) {
        return -EIO;
    }

    int32_t distance = (int32_t)((pulse_width * 343) / 2000);
    *raw = distance;
    return 0;
}