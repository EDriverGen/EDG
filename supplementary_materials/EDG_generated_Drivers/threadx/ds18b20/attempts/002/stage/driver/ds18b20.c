#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

#define DS18B20_GPIO_PORT GPIOA
#define DS18B20_DQ_PIN GPIO_PIN_0

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        HAL_Delay(1);
    }
}

static void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

static void set_dq_output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_GPIO_PORT, &GPIO_InitStruct);
}

static void set_dq_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DS18B20_GPIO_PORT, &GPIO_InitStruct);
}

static void dq_low(void) {
    HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
}

static void dq_high(void) {
    HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
}

static int dq_read(void) {
    return (HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_DQ_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

static int reset_pulse(void) {
    set_dq_output();
    dq_low();
    delay_us(480);
    set_dq_input();
    delay_us(60);
    int presence = 0;
    if (dq_read() == 0) {
        presence = 1;
    }
    delay_us(420);
    return presence;
}

static void write_bit(int bit) {
    set_dq_output();
    dq_low();
    if (bit) {
        delay_us(1);
        dq_high();
        delay_us(60);
    } else {
        delay_us(60);
        dq_high();
        delay_us(1);
    }
}

static int read_bit(void) {
    int bit;
    set_dq_output();
    dq_low();
    delay_us(1);
    set_dq_input();
    delay_us(1);
    bit = dq_read();
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

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    if (!reset_pulse()) {
        return -1;
    }
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    (void)dev;
    if (!reset_pulse()) {
        return -1;
    }
    write_byte(0xCC);
    write_byte(0x44);
    delay_ms(750);
    if (!reset_pulse()) {
        return -1;
    }
    write_byte(0xCC);
    write_byte(0xBE);
    uint8_t lsb = read_byte();
    uint8_t msb = read_byte();
    int16_t temp_raw = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)temp_raw * 625 / 10;
    return 0;
}