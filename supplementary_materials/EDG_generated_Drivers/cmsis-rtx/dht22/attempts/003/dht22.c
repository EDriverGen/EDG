#include "dht22.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#define DHT22_DATA_PIN GPIO_PIN_5
#define DHT22_DATA_PORT GPIOB

void dht22_init(struct dht22_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = DHT22_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_DATA_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT22_DATA_PORT, DHT22_DATA_PIN, GPIO_PIN_SET);
    HAL_Delay(1000);
}

static int dht22_read_bit(GPIO_TypeDef *port, uint16_t pin, uint32_t timeout_us) {
    uint32_t count = 0;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        if (count++ >= timeout_us) return -1;
        for (volatile int i = 0; i < 10; i++);
    }
    count = 0;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        if (count++ >= timeout_us) return -1;
        for (volatile int i = 0; i < 10; i++);
    }
    return (count > 30) ? 1 : 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (!dev || !humidity_val || !temp_val) return -1;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_DATA_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT22_DATA_PORT, DHT22_DATA_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT22_DATA_PORT, DHT22_DATA_PIN, GPIO_PIN_SET);
    for (volatile int i = 0; i < 30; i++);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT22_DATA_PORT, &GPIO_InitStruct);
    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_SET) {
        if (++timeout > 1000) return -1;
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_RESET) {
        if (++timeout > 1000) return -1;
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_SET) {
        if (++timeout > 1000) return -1;
    }
    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_RESET);
        uint32_t high_time = 0;
        while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_SET) {
            high_time++;
            for (volatile int j = 0; j < 10; j++);
        }
        data[i / 8] <<= 1;
        if (high_time > 30) data[i / 8] |= 1;
    }
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    *humidity_val = (int32_t)raw_hum * 100 + (int32_t)(raw_hum & 0xFF);
    *temp_val = (int32_t)raw_temp * 100 + (int32_t)(raw_temp & 0xFF);
    return 0;
}