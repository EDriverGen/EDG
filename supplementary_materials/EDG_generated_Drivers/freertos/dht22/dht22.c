#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include <stddef.h>

#define DHT22_TIMEOUT_US 200

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 12; j++) {}
    }
}

static int wait_for_pin_state(GPIO_TypeDef *GPIOx, uint16_t pin, GPIO_PinState state, uint32_t timeout_us) {
    while (timeout_us--) {
        if (HAL_GPIO_ReadPin(GPIOx, pin) == state) return 0;
        delay_us(1);
    }
    return -1;
}

int dht22_init(struct dht22_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->trig_pin = GPIO_PIN_5;
    dev->echo_pin = GPIO_PIN_5;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = dev->trig_pin | dev->echo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, dev->trig_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, dev->echo_pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (!dev || !humidity_val || !temp_val) return -1;
    GPIO_TypeDef *GPIOx = GPIOB;
    uint16_t trig = dev->trig_pin;
    uint16_t echo = dev->echo_pin;
    
    // Set both pins as output, low for start signal
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = trig | echo;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOx, trig, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOx, echo, GPIO_PIN_RESET);
    HAL_Delay(18);
    
    // Pull up and wait
    HAL_GPIO_WritePin(GPIOx, trig, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOx, echo, GPIO_PIN_SET);
    delay_us(30);
    
    // Set as input
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
    
    // Wait for sensor response low
    if (wait_for_pin_state(GPIOx, echo, GPIO_PIN_RESET, DHT22_TIMEOUT_US) != 0) return -1;
    // Wait for response high
    if (wait_for_pin_state(GPIOx, echo, GPIO_PIN_SET, DHT22_TIMEOUT_US) != 0) return -1;
    
    // Read 40 bits
    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        // Wait for low
        if (wait_for_pin_state(GPIOx, echo, GPIO_PIN_RESET, DHT22_TIMEOUT_US) != 0) return -1;
        // Wait for high
        if (wait_for_pin_state(GPIOx, echo, GPIO_PIN_SET, DHT22_TIMEOUT_US) != 0) return -1;
        // Measure high pulse width
        uint32_t width = 0;
        while (HAL_GPIO_ReadPin(GPIOx, echo) == GPIO_PIN_SET && width < DHT22_TIMEOUT_US) {
            width++;
            delay_us(1);
        }
        if (width >= DHT22_TIMEOUT_US) return -1;
        // Classify bit: if width > 40us then 1 else 0
        if (width > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    
    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;
    
    // Convert to milli units
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    int32_t temp_x10 = (raw_temp & 0x8000)
        ? -(int32_t)(raw_temp & 0x7FFF)
        : (int32_t)raw_temp;
    *humidity_val = (int32_t)raw_hum * 100;
    *temp_val = temp_x10 * 100;
    
    return 0;
}
