#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <stdint.h>

#define DHT22_TRIG_PIN GPIO_PIN_0
#define DHT22_ECHO_PIN GPIO_PIN_1
#define DHT22_GPIO_PORT GPIOA

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        HAL_Delay(1);
    }
}

static int wait_for_pin_state(GPIO_PinState state, uint32_t timeout_us) {
    while (timeout_us--) {
        if (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_ECHO_PIN) == state) {
            return 0;
        }
        delay_us(1);
    }
    return -1;
}

int dht22_init(struct dht22_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->trig_pin = 0;
    dev->echo_pin = 1;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = DHT22_TRIG_PIN | DHT22_ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_TRIG_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_ECHO_PIN, GPIO_PIN_SET);

    HAL_Delay(1000);

    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (!dev || !humidity_val || !temp_val) return -1;

    uint8_t data[5] = {0};
    int bit_index;
    int byte_index;

    // Send start signal
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_TRIG_PIN, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_TRIG_PIN, GPIO_PIN_SET);
    delay_us(30);

    // Wait for sensor response
    if (wait_for_pin_state(GPIO_PIN_RESET, 100) != 0) return -1;
    if (wait_for_pin_state(GPIO_PIN_SET, 100) != 0) return -1;
    if (wait_for_pin_state(GPIO_PIN_RESET, 100) != 0) return -1;

    // Read 40 bits
    for (bit_index = 0; bit_index < 40; bit_index++) {
        if (wait_for_pin_state(GPIO_PIN_SET, 100) != 0) return -1;
        uint32_t high_time = 0;
        while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_ECHO_PIN) == GPIO_PIN_SET) {
            high_time++;
            delay_us(1);
            if (high_time > 100) return -1;
        }
        if (high_time > 50) {
            data[bit_index / 8] |= (1 << (7 - (bit_index % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;

    // Convert to milli units
    uint16_t integral_rh = (data[0] << 8) | data[1];
    uint16_t integral_t = (data[2] << 8) | data[3];

    *humidity_val = (int32_t)integral_rh * 100 + (int32_t)integral_rh;
    *temp_val = (int32_t)integral_t * 100 + (int32_t)integral_t;

    return 0;
}