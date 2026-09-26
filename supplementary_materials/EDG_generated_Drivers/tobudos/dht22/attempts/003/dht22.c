#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <stdint.h>

#define DHT22_TRIG_PIN GPIO_PIN_0
#define DHT22_ECHO_PIN GPIO_PIN_1
#define DHT22_GPIO_PORT GPIOA

#define DHT22_TIMEOUT_US 1000

static void delay_us(uint32_t us) {
    uint32_t ticks = us * (SystemCoreClock / 1000000) / 2;
    for (uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

static int wait_for_pin_state(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout_us) {
    while (timeout_us--) {
        if (HAL_GPIO_ReadPin(port, pin) == state) return 0;
        delay_us(1);
    }
    return -1;
}

int dht22_init(struct dht22_device *dev, void *bus_name) {
    (void)bus_name;
    dev->bus_handle = (void *)DHT22_GPIO_PORT;
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

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    (void)dev;
    uint8_t data[5] = {0};
    uint8_t bit_index = 0;
    uint8_t byte_index = 0;

    // Send start signal
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_TRIG_PIN, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_TRIG_PIN, GPIO_PIN_SET);

    // Wait for sensor response
    delay_us(30);

    // Switch to input
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);

    // Wait for response low (80us)
    if (wait_for_pin_state(DHT22_GPIO_PORT, DHT22_ECHO_PIN, GPIO_PIN_RESET, DHT22_TIMEOUT_US) != 0) {
        return -1;
    }
    // Wait for response high (80us)
    if (wait_for_pin_state(DHT22_GPIO_PORT, DHT22_ECHO_PIN, GPIO_PIN_SET, DHT22_TIMEOUT_US) != 0) {
        return -1;
    }

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        // Wait for low (50us)
        if (wait_for_pin_state(DHT22_GPIO_PORT, DHT22_ECHO_PIN, GPIO_PIN_RESET, DHT22_TIMEOUT_US) != 0) {
            return -1;
        }
        // Wait for high
        if (wait_for_pin_state(DHT22_GPIO_PORT, DHT22_ECHO_PIN, GPIO_PIN_SET, DHT22_TIMEOUT_US) != 0) {
            return -1;
        }
        // Measure high pulse width
        uint32_t count = 0;
        while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_ECHO_PIN) == GPIO_PIN_SET && count < DHT22_TIMEOUT_US) {
            delay_us(1);
            count++;
        }
        if (count > 50) {
            data[byte_index] |= (1 << (7 - bit_index));
        }
        bit_index++;
        if (bit_index == 8) {
            bit_index = 0;
            byte_index++;
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -1;
    }

    // Convert humidity
    uint16_t integral_RH = (uint16_t)data[0] << 8 | data[1];
    uint16_t decimal_RH = (uint16_t)data[0] << 8 | data[1];
    *humidity_val = (int32_t)integral_RH * 100 + (int32_t)decimal_RH;

    // Convert temperature
    uint16_t integral_T = (uint16_t)data[2] << 8 | data[3];
    uint16_t decimal_T = (uint16_t)data[2] << 8 | data[3];
    *temp_val = (int32_t)integral_T * 100 + (int32_t)decimal_T;

    return 0;
}