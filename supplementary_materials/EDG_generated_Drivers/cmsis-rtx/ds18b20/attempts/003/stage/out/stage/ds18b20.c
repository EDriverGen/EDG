#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <stdint.h>

#define DS18B20_DQ_PORT GPIOB
#define DS18B20_DQ_PIN GPIO_PIN_5

#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

#define DS18B20_CONVERSION_DELAY_MS 750

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 12; j++) {}
    }
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, uint8_t bit) {
    GPIO_TypeDef *port = DS18B20_DQ_PORT;
    uint16_t pin = DS18B20_DQ_PIN;
    if (bit) {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
        delay_us(1);
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        delay_us(60);
    } else {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
        delay_us(60);
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        delay_us(1);
    }
}

static uint8_t ds18b20_read_bit(struct ds18b20_dev *dev) {
    GPIO_TypeDef *port = DS18B20_DQ_PORT;
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
    if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        return -1;
    }
    delay_us(410);
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    return ds18b20_reset(dev);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, 0xCC);
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    HAL_Delay(DS18B20_CONVERSION_DELAY_MS);
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, 0xCC);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}