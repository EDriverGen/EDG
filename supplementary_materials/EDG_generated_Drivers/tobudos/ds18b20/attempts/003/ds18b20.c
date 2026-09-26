#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

#define DS18B20_DQ_PORT GPIOA
#define DS18B20_DQ_PIN GPIO_PIN_0

static void delay_us(uint32_t us) {
    uint32_t ticks = us * (SystemCoreClock / 1000000) / 3;
    for (uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, uint8_t bit) {
    GPIO_TypeDef *port = DS18B20_DQ_PORT;
    uint16_t pin = DS18B20_DQ_PIN;
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    if (bit) {
        delay_us(5);
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        delay_us(65);
    } else {
        delay_us(65);
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        delay_us(5);
    }
}

static uint8_t ds18b20_read_bit(struct ds18b20_dev *dev) {
    GPIO_TypeDef *port = DS18B20_DQ_PORT;
    uint16_t pin = DS18B20_DQ_PIN;
    uint8_t bit;
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    delay_us(5);
    bit = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1 : 0;
    delay_us(55);
    return bit;
}

static void ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(dev, (byte >> i) & 0x01);
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
    GPIO_TypeDef *port = DS18B20_DQ_PORT;
    uint16_t pin = DS18B20_DQ_PIN;
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    delay_us(480);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    delay_us(70);
    uint8_t presence = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1 : 0;
    delay_us(410);
    return presence ? 0 : -1;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    HAL_Delay(1);
    return ds18b20_reset(dev);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    int ret;
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, 0xCC);
    ds18b20_write_byte(dev, 0x44);
    HAL_Delay(750);
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, 0xCC);
    ds18b20_write_byte(dev, 0xBE);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}