#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include <stddef.h>

#define DHT22_TRIG_PIN GPIO_PIN_0
#define DHT22_ECHO_PIN GPIO_PIN_1
#define DHT22_GPIO_PORT GPIOA

#define DHT22_TIMEOUT_US 1000

static void delay_us(uint32_t us) {
    HAL_Delay(1);
    // Note: HAL_Delay has millisecond resolution; for microsecond delays we use a simple loop
    // This is a workaround; actual microsecond delay would require a timer
    for (uint32_t i = 0; i < us * 10; i++) {
        __NOP();
    }
}

static int dht22_wait_for_pin_state(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout_us) {
    uint32_t count = 0;
    while (HAL_GPIO_ReadPin(port, pin) != state) {
        if (count++ >= timeout_us) {
            return -1;
        }
        delay_us(1);
    }
    return 0;
}

int dht22_init(struct dht22_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->trig_pin = DHT22_TRIG_PIN;
    dev->echo_pin = DHT22_ECHO_PIN;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_TRIG_PIN | DHT22_ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_TRIG_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_ECHO_PIN, GPIO_PIN_SET);

    HAL_Delay(1000);

    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity, int32_t *temperature) {
    if (!dev || !humidity || !temperature) {
        return -1;
    }

    GPIO_TypeDef *port = DHT22_GPIO_PORT;
    uint16_t trig = dev->trig_pin;
    uint16_t echo = dev->echo_pin;

    // Set both pins as output initially
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = trig | echo;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // Send start signal: pull low for at least 18ms
    HAL_GPIO_WritePin(port, trig, GPIO_PIN_RESET);
    HAL_Delay(20); // 20ms > 18ms

    // Pull up and wait 20-40us
    HAL_GPIO_WritePin(port, trig, GPIO_PIN_SET);
    delay_us(30);

    // Set echo pin as input
    GPIO_InitStruct.Pin = echo;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // Wait for sensor response: low for 80us
    if (dht22_wait_for_pin_state(port, echo, GPIO_PIN_RESET, 200) != 0) {
        return -1;
    }
    delay_us(80);

    // Wait for high for 80us
    if (dht22_wait_for_pin_state(port, echo, GPIO_PIN_SET, 200) != 0) {
        return -1;
    }
    delay_us(80);

    // Read 40 bits
    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        // Wait for low (50us)
        if (dht22_wait_for_pin_state(port, echo, GPIO_PIN_RESET, 200) != 0) {
            return -1;
        }
        delay_us(50);

        // Wait for high
        if (dht22_wait_for_pin_state(port, echo, GPIO_PIN_SET, 200) != 0) {
            return -1;
        }

        // Measure high pulse width
        uint32_t high_start = 0;
        while (HAL_GPIO_ReadPin(port, echo) == GPIO_PIN_SET) {
            high_start++;
            delay_us(1);
            if (high_start > 200) {
                return -1;
            }
        }

        // Determine bit: if high pulse > 50us, it's 1; else 0
        if (high_start > 50) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -1;
    }

    // Convert to milli units
    uint16_t integral_RH = data[0];
    uint16_t decimal_RH = data[1];
    uint16_t integral_T = data[2];
    uint16_t decimal_T = data[3];

    *humidity = (int32_t)(integral_RH * 100 + decimal_RH);
    *temperature = (int32_t)(integral_T * 100 + decimal_T);

    return 0;
}