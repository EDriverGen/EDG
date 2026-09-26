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

static void write_bit(struct ds18b20_dev *dev, uint8_t bit) {
    (void)dev;
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us(6);
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
        delay_us(64);
    } else {
        delay_us(60);
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
        delay_us(10);
    }
}

static uint8_t read_bit(struct ds18b20_dev *dev) {
    (void)dev;
    uint8_t bit;
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
    delay_us(6);
    bit = (HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN) == GPIO_PIN_SET) ? 1 : 0;
    delay_us(55);
    return bit;
}

static void write_byte(struct ds18b20_dev *dev, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        write_bit(dev, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t read_byte(struct ds18b20_dev *dev) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (read_bit(dev)) {
            byte |= 0x80;
        }
    }
    return byte;
}

static int reset_pulse(struct ds18b20_dev *dev) {
    (void)dev;
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    if (HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN) == GPIO_PIN_RESET) {
        return 0;
    }
    return -1;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    return reset_pulse(dev);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    if (reset_pulse(dev) != 0) {
        return -1;
    }
    write_byte(dev, 0xCC);
    write_byte(dev, 0x44);
    HAL_Delay(750);
    if (reset_pulse(dev) != 0) {
        return -1;
    }
    write_byte(dev, 0xCC);
    write_byte(dev, 0xBE);
    uint8_t lsb = read_byte(dev);
    uint8_t msb = read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}