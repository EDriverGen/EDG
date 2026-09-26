#include "dht22.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_gpio.h"
#define DHT22_TIMEOUT 1000

int dht22_init(struct dht22_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->port = GPIOA;
    dev->pin = GPIO_PIN_0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_Delay(1000);
    return 0;
}

static int dht22_wait_for_pin_state(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout_us) {
    uint32_t count = 0;
    while (HAL_GPIO_ReadPin(port, pin) != state) {
        HAL_Delay(0);
        count++;
        if (count > timeout_us) return -1;
    }
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (!dev || !humidity_val || !temp_val) return -1;
    GPIO_TypeDef *port = dev->port;
    uint16_t pin = dev->pin;
    uint8_t data[5] = {0};
    int bit_index;
    uint32_t high_time;
    
    // Send start signal
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    HAL_Delay(0); // 20-40us wait, use microsecond delay approximation
    // Wait for sensor response (pull low)
    if (dht22_wait_for_pin_state(port, pin, GPIO_PIN_RESET, 200) != 0) return -1;
    // Wait for response low duration (80us)
    if (dht22_wait_for_pin_state(port, pin, GPIO_PIN_SET, 200) != 0) return -1;
    // Wait for response high duration (80us)
    if (dht22_wait_for_pin_state(port, pin, GPIO_PIN_RESET, 200) != 0) return -1;
    
    // Read 40 bits
    for (bit_index = 0; bit_index < 40; bit_index++) {
        // Wait for low (50us)
        if (dht22_wait_for_pin_state(port, pin, GPIO_PIN_SET, 200) != 0) return -1;
        // Measure high time
        uint32_t high_start = 0;
        while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
            high_start++;
            if (high_start > 200) return -1;
        }
        high_time = high_start;
        if (high_time > 50) {
            data[bit_index / 8] |= (1 << (7 - (bit_index % 8)));
        }
    }
    
    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;
    
    // Convert to milli units
    uint16_t integral_RH = data[0];
    uint16_t decimal_RH = data[1];
    uint16_t integral_T = data[2];
    uint16_t decimal_T = data[3];
    
    *humidity_val = (int32_t)(integral_RH * 100 + decimal_RH);
    *temp_val = (int32_t)(integral_T * 100 + decimal_T);
    
    return 0;
}
