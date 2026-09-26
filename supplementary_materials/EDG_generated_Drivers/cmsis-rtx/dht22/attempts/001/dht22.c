#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include <stddef.h>

#define DHT22_DATA_PIN GPIO_PIN_5
#define DHT22_DATA_PORT GPIOB

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 12; j++) {}
    }
}

void dht22_init(struct dht22_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = DHT22_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_DATA_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT22_DATA_PORT, DHT22_DATA_PIN, GPIO_PIN_SET);
    HAL_Delay(1000);
}

static int read_bit(void) {
    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_RESET) {
        if (++timeout > 100) return -1;
        delay_us(1);
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_SET) {
        if (++timeout > 100) return -1;
        delay_us(1);
    }
    uint32_t high_time = timeout;
    if (high_time > 40) return 1;
    else return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (dev == NULL || humidity_val == NULL || temp_val == NULL) return -1;
    uint8_t data[5] = {0};
    HAL_GPIO_WritePin(DHT22_DATA_PORT, DHT22_DATA_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT22_DATA_PORT, DHT22_DATA_PIN, GPIO_PIN_SET);
    delay_us(30);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT22_DATA_PORT, &GPIO_InitStruct);
    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_SET) {
        if (++timeout > 100) return -1;
        delay_us(1);
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_RESET) {
        if (++timeout > 200) return -1;
        delay_us(1);
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_DATA_PORT, DHT22_DATA_PIN) == GPIO_PIN_SET) {
        if (++timeout > 200) return -1;
        delay_us(1);
    }
    for (int i = 0; i < 40; i++) {
        int bit = read_bit();
        if (bit < 0) return -1;
        data[i / 8] = (data[i / 8] << 1) | bit;
    }
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    *humidity_val = (int32_t)raw_hum * 100 + (int32_t)raw_hum;
    *temp_val = (int32_t)raw_temp * 100 + (int32_t)raw_temp;
    return 0;
}