#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_gpio.h"
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

static void delay_us(uint32_t us) {
    uint32_t ticks = us * (SystemCoreClock / 1000000) / 3;
    for (uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

static int ds18b20_reset(struct ds18b20_dev *dev) {
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    HAL_Delay(1); // at least 480 us, use 1ms for simplicity
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
    delay_us(60); // wait for presence pulse
    GPIO_PinState presence = HAL_GPIO_ReadPin(dev->port, dev->pin);
    if (presence != GPIO_PIN_RESET) {
        return -1; // no presence
    }
    delay_us(240);
    return 0;
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, uint8_t bit) {
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    if (bit) {
        delay_us(15);
        HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
        delay_us(105);
    } else {
        delay_us(120);
        HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
        delay_us(1);
    }
}

static uint8_t ds18b20_read_bit(struct ds18b20_dev *dev) {
    uint8_t bit = 0;
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    delay_us(1);
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
    delay_us(10);
    if (HAL_GPIO_ReadPin(dev->port, dev->pin) != GPIO_PIN_RESET) {
        bit = 1;
    }
    delay_us(109);
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

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    // bus_handle is not used; GPIO pins are fixed
    dev->port = GPIOA;
    dev->pin = GPIO_PIN_0;
    // Configure GPIO as output open-drain with pull-up
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    // Set pin high initially
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    // Reset and presence pulse
    if (ds18b20_reset(dev) != 0) {
        return -1;
    }
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    // Skip ROM
    ds18b20_reset(dev);
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    // Convert T
    ds18b20_write_byte(dev, DS18B20_CONVERT_T);
    // Wait for conversion (12-bit: 750ms)
    HAL_Delay(750);
    // Read scratchpad
    ds18b20_reset(dev);
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    // Sign extend from 12 bits
    if (raw_temp & 0x0800) {
        raw_temp |= 0xF000;
    } else {
        raw_temp &= 0x0FFF;
    }
    // Convert to milli-degrees: raw * 625 / 10
    *raw = ((int32_t)raw_temp * 625) / 10;
    return 0;
}
