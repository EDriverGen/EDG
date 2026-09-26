#include "dht22.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_gpio.h"
#define DHT22_PORT GPIOB
#define DHT22_PIN GPIO_PIN_5

static int dht22_wait_for_state(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout_us)
{
    uint32_t delay = 0;
    while (HAL_GPIO_ReadPin(port, pin) != state) {
        HAL_Delay(0);
        delay++;
        if (delay > timeout_us) return -1;
    }
    return 0;
}

int dht22_init(struct dht22_device *dev, void *bus_name)
{
    (void)bus_name;
    dev->port = DHT22_PORT;
    dev->pin = DHT22_PIN;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = dev->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(dev->port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
    HAL_Delay(1000);

    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    uint8_t data[5] = {0};
    uint32_t timeout_us = 200;

    // Send start signal: pull low for at least 18ms
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    HAL_Delay(20);

    // Release line and wait for sensor response
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
    HAL_Delay(0); // 30us wait approximated

    // Wait for sensor to pull low (response)
    if (dht22_wait_for_state(dev->port, dev->pin, GPIO_PIN_RESET, timeout_us) != 0) {
        return -1;
    }
    // Wait for sensor to pull high
    if (dht22_wait_for_state(dev->port, dev->pin, GPIO_PIN_SET, timeout_us) != 0) {
        return -1;
    }

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        // Wait for low (start of bit)
        if (dht22_wait_for_state(dev->port, dev->pin, GPIO_PIN_RESET, timeout_us) != 0) {
            return -1;
        }
        // Wait for high
        if (dht22_wait_for_state(dev->port, dev->pin, GPIO_PIN_SET, timeout_us) != 0) {
            return -1;
        }
        // Measure high pulse width
        uint32_t high_start = 0;
        while (HAL_GPIO_ReadPin(dev->port, dev->pin) == GPIO_PIN_SET) {
            high_start++;
            if (high_start > 100) break;
        }
        // Determine bit: if high pulse > 40us (approx), bit=1 else 0
        if (high_start > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -1;
    }

    // Extract integral and decimal parts
    uint16_t integral_RH = data[0];
    uint16_t decimal_RH = data[1];
    uint16_t integral_T = data[2];
    uint16_t decimal_T = data[3];

    // Convert to milli units using integer approximation
    int32_t raw_humidity = (integral_RH << 8) | decimal_RH;
    int32_t raw_temp = (integral_T << 8) | decimal_T;

    int32_t temp_x10 = (raw_temp & 0x8000)
        ? -(int32_t)(raw_temp & 0x7FFF)
        : raw_temp;

    *humidity_val = raw_humidity * 100;
    *temp_val = temp_x10 * 100;

    return 0;
}
