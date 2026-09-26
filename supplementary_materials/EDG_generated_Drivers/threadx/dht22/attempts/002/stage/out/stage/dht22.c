#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <stdint.h>

#define DHT22_TIMEOUT 1000

static int dht22_wait_for_pin_state(struct dht22_device *dev, GPIO_PinState state, uint32_t timeout_us)
{
    uint32_t delay = 0;
    while (HAL_GPIO_ReadPin((GPIO_TypeDef *)dev->GPIOx, dev->echo_pin) != state) {
        if (delay >= timeout_us) return -1;
        HAL_Delay(1);
        delay += 1;
    }
    return 0;
}

int dht22_init(struct dht22_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->GPIOx = (void *)GPIOA;
    dev->trig_pin = GPIO_PIN_0;
    dev->echo_pin = GPIO_PIN_1;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = dev->trig_pin | dev->echo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)dev->GPIOx, &GPIO_InitStruct);

    HAL_GPIO_WritePin((GPIO_TypeDef *)dev->GPIOx, dev->trig_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin((GPIO_TypeDef *)dev->GPIOx, dev->echo_pin, GPIO_PIN_SET);

    HAL_Delay(1000);

    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    uint8_t data[5] = {0};
    uint32_t timeout;
    int bit_idx, byte_idx;
    uint32_t pulse_width;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = dev->trig_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)dev->GPIOx, &GPIO_InitStruct);

    HAL_GPIO_WritePin((GPIO_TypeDef *)dev->GPIOx, dev->trig_pin, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin((GPIO_TypeDef *)dev->GPIOx, dev->trig_pin, GPIO_PIN_SET);
    HAL_Delay(1);

    GPIO_InitStruct.Pin = dev->echo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init((GPIO_TypeDef *)dev->GPIOx, &GPIO_InitStruct);

    timeout = 0;
    while (HAL_GPIO_ReadPin((GPIO_TypeDef *)dev->GPIOx, dev->echo_pin) == GPIO_PIN_SET) {
        if (++timeout > 100) return -1;
        HAL_Delay(1);
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin((GPIO_TypeDef *)dev->GPIOx, dev->echo_pin) == GPIO_PIN_RESET) {
        if (++timeout > 100) return -1;
        HAL_Delay(1);
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin((GPIO_TypeDef *)dev->GPIOx, dev->echo_pin) == GPIO_PIN_SET) {
        if (++timeout > 100) return -1;
        HAL_Delay(1);
    }

    for (byte_idx = 0; byte_idx < 5; byte_idx++) {
        for (bit_idx = 7; bit_idx >= 0; bit_idx--) {
            timeout = 0;
            while (HAL_GPIO_ReadPin((GPIO_TypeDef *)dev->GPIOx, dev->echo_pin) == GPIO_PIN_RESET) {
                if (++timeout > 100) return -1;
                HAL_Delay(1);
            }

            timeout = 0;
            while (HAL_GPIO_ReadPin((GPIO_TypeDef *)dev->GPIOx, dev->echo_pin) == GPIO_PIN_SET) {
                if (++timeout > 100) return -1;
                HAL_Delay(1);
            }

            pulse_width = timeout;
            if (pulse_width > 50) {
                data[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;

    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    *humidity_val = (int32_t)raw_hum * 100 + (int32_t)raw_hum;
    *temp_val = (int32_t)raw_temp * 100 + (int32_t)raw_temp;

    return 0;
}