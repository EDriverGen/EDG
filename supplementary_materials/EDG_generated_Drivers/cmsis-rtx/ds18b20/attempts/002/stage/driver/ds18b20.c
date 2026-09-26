#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

#define DQ_PORT GPIOB
#define DQ_PIN GPIO_PIN_5

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 8; j++) {}
    }
}

static void set_output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DQ_PORT, &GPIO_InitStruct);
}

static void set_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DQ_PORT, &GPIO_InitStruct);
}

static void write_bit(int bit) {
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us(1);
        HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
        delay_us(60);
    } else {
        delay_us(60);
        HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
        delay_us(1);
    }
}

static int read_bit(void) {
    int bit;
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_RESET);
    delay_us(1);
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
    delay_us(1);
    bit = (HAL_GPIO_ReadPin(DQ_PORT, DQ_PIN) == GPIO_PIN_SET) ? 1 : 0;
    delay_us(60);
    return bit;
}

static void write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t read_byte(void) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (read_bit()) {
            byte |= 0x80;
        }
    }
    return byte;
}

static int reset_pulse(void) {
    set_output();
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    set_input();
    HAL_Delay(1);
    if (HAL_GPIO_ReadPin(DQ_PORT, DQ_PIN) == GPIO_PIN_SET) {
        return -1;
    }
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    if (reset_pulse() != 0) {
        return -1;
    }
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    (void)dev;
    if (reset_pulse() != 0) {
        return -1;
    }
    write_byte(DS18B20_SKIP_ROM);
    write_byte(DS18B20_CONVERT_T);
    HAL_Delay(750);
    if (reset_pulse() != 0) {
        return -1;
    }
    write_byte(DS18B20_SKIP_ROM);
    write_byte(DS18B20_READ_SCRATCHPAD);
    uint8_t lsb = read_byte();
    uint8_t msb = read_byte();
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}