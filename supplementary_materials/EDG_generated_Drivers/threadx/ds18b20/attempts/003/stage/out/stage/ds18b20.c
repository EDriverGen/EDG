#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <stdint.h>

#define DS18B20_GPIO_PORT GPIOA
#define DS18B20_DQ_PIN GPIO_PIN_0

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 8; j++) {}
    }
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, uint8_t bit) {
    GPIO_TypeDef *port = DS18B20_GPIO_PORT;
    uint16_t pin = DS18B20_DQ_PIN;
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    if (bit) {
        delay_us(1);
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        delay_us(60);
    } else {
        delay_us(60);
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        delay_us(1);
    }
}

static uint8_t ds18b20_read_bit(struct ds18b20_dev *dev) {
    GPIO_TypeDef *port = DS18B20_GPIO_PORT;
    uint16_t pin = DS18B20_DQ_PIN;
    uint8_t bit;
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    delay_us(1);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    delay_us(1);
    bit = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1 : 0;
    delay_us(60);
    return bit;
}

static void ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(dev, (byte >> i) & 1);
    }
}

static uint8_t ds18b20_read_byte(struct ds18b20_dev *dev) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (ds18b20_read_bit(dev)) {
            byte |= (1 << i);
        }
    }
    return byte;
}

static int ds18b20_reset(struct ds18b20_dev *dev) {
    GPIO_TypeDef *port = DS18B20_GPIO_PORT;
    uint16_t pin = DS18B20_DQ_PIN;
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    delay_us(480);
    uint8_t presence = 0;
    for (int i = 0; i < 100; i++) {
        if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
            presence = 1;
            break;
        }
        delay_us(10);
    }
    if (!presence) return -1;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        delay_us(10);
    }
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = DS18B20_DQ_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_GPIO_PORT, &gpio_init);
    HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
    if (ds18b20_reset(dev) != 0) return -1;
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, 0xCC);
    ds18b20_write_byte(dev, 0x44);
    HAL_Delay(750);
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, 0xCC);
    ds18b20_write_byte(dev, 0xBE);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw16 = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw16 * 625 / 10;
    return 0;
}