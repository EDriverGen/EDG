#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

#define TRIG_PORT GPIOA
#define TRIG_PIN GPIO_PIN_0
#define ECHO_PORT GPIOA
#define ECHO_PIN GPIO_PIN_1

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        HAL_Delay(1);
    }
}

static void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

static void set_pin_output(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void set_pin_input(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static int ds18b20_reset(struct ds18b20_dev *dev) {
    (void)dev;
    set_pin_output(TRIG_PORT, TRIG_PIN);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    delay_us(480);
    set_pin_input(TRIG_PORT, TRIG_PIN);
    delay_us(60);
    if (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET) {
        uint32_t timeout = 240;
        while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET && timeout--) {
            delay_us(1);
        }
        if (timeout == 0) return -1;
        return 0;
    }
    return -1;
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, uint8_t bit) {
    (void)dev;
    set_pin_output(TRIG_PORT, TRIG_PIN);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us(1);
        HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
        delay_us(60);
    } else {
        delay_us(60);
        HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
        delay_us(1);
    }
    set_pin_input(TRIG_PORT, TRIG_PIN);
}

static uint8_t ds18b20_read_bit(struct ds18b20_dev *dev) {
    (void)dev;
    uint8_t bit = 0;
    set_pin_output(TRIG_PORT, TRIG_PIN);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    delay_us(1);
    set_pin_input(TRIG_PORT, TRIG_PIN);
    delay_us(1);
    if (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_SET) {
        bit = 1;
    }
    delay_us(60);
    return bit;
}

static void ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(dev, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(struct ds18b20_dev *dev) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (ds18b20_read_bit(dev)) {
            byte |= 0x80;
        }
    }
    return byte;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->trig_pin = TRIG_PIN;
    dev->echo_pin = ECHO_PIN;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = TRIG_PIN | ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ECHO_PORT, ECHO_PIN, GPIO_PIN_SET);
    if (ds18b20_reset(dev) != 0) {
        return -1;
    }
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    if (ds18b20_reset(dev) != 0) {
        return -1;
    }
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CONVERT_T);
    delay_ms(750);
    if (ds18b20_reset(dev) != 0) {
        return -1;
    }
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw16 = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw16 * 625 / 10;
    return 0;
}