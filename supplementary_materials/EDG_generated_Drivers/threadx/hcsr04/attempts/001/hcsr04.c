#include "hcsr04.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <errno.h>

#define TRIG_PORT GPIOA
#define TRIG_PIN GPIO_PIN_0
#define ECHO_PORT GPIOA
#define ECHO_PIN GPIO_PIN_1

static void delay_us(uint32_t us) {
    // Approximate microsecond delay using HAL_Delay (milliseconds) for simplicity
    // but for 10 us we need a more precise delay; use a simple loop
    // Since HAL_Delay only supports ms, we implement a busy-wait loop
    // This is acceptable for short delays
    uint32_t count = us * 8; // rough calibration for 72 MHz
    while (count--) {
        __NOP();
    }
}

int hcsr04_init(struct hcsr04_device *dev, void *bus_handle) {
    (void)bus_handle;
    dev->trig_port = TRIG_PORT;
    dev->trig_pin = TRIG_PIN;
    dev->echo_port = ECHO_PORT;
    dev->echo_pin = ECHO_PIN;

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure TRIG pin as output
    GPIO_InitStruct.Pin = TRIG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);

    // Configure ECHO pin as input
    GPIO_InitStruct.Pin = ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ECHO_PORT, &GPIO_InitStruct);

    // Ensure TRIG is low
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw) {
    uint32_t timeout;
    GPIO_PinState state;
    uint32_t pulse_width_us = 0;

    // Send trigger pulse: 10 us high
    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(dev->trig_port, dev->trig_pin, GPIO_PIN_RESET);

    // Wait for echo pin to go high (start of pulse)
    timeout = 1000000; // 1 second timeout
    while (HAL_GPIO_ReadPin(dev->echo_port, dev->echo_pin) == GPIO_PIN_RESET) {
        if (--timeout == 0) {
            return -EIO;
        }
    }

    // Measure pulse width
    timeout = 1000000;
    while (HAL_GPIO_ReadPin(dev->echo_port, dev->echo_pin) == GPIO_PIN_SET) {
        if (--timeout == 0) {
            return -EIO;
        }
        pulse_width_us++;
        delay_us(1);
    }

    // Convert to distance in mm: (T * 343) // 2000
    *raw = (int32_t)((pulse_width_us * 343) / 2000);

    return 0;
}