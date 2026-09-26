#include "ds18b20.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

#define DS18B20_DQ_PORT GPIOB
#define DS18B20_DQ_PIN GPIO_PIN_5

#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_SKIP_ROM 0xCC

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 8; j++) {}
    }
}

static void set_output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_DQ_PORT, &GPIO_InitStruct);
}

static void set_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS18B20_DQ_PORT, &GPIO_InitStruct);
}

static int ds18b20_reset(struct ds18b20_dev *dev) {
    (void)dev;
    set_output();
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
    HAL_Delay(1); // at least 480 us, use 1ms
    set_input();
    delay_us(60); // wait for presence pulse window
    if (HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN) != GPIO_PIN_RESET) {
        return -1; // no presence pulse
    }
    delay_us(240);
    if (HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN) != GPIO_PIN_SET) {
        return -1; // presence pulse too long
    }
    return 0;
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, uint8_t bit) {
    (void)dev;
    set_output();
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us(5);
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
        delay_us(60);
    } else {
        delay_us(60);
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
        delay_us(5);
    }
    set_input();
}

static uint8_t ds18b20_read_bit(struct ds18b20_dev *dev) {
    (void)dev;
    uint8_t bit;
    set_output();
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
    delay_us(2);
    set_input();
    delay_us(5);
    bit = (HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN) == GPIO_PIN_SET) ? 1 : 0;
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

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    return ds18b20_reset(dev);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    int ret;
    // Skip ROM
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    // Convert T
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    // Wait for conversion (12-bit: 750ms)
    HAL_Delay(750);
    // Reset
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    // Skip ROM
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    // Read Scratchpad
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw16 = (int16_t)((msb << 8) | lsb);
    // Sign extend from bit 11 (12-bit resolution)
    if (raw16 & 0x0800) {
        raw16 |= 0xF000;
    } else {
        raw16 &= 0x0FFF;
    }
    // Convert to milli-degrees: raw * 625 / 10
    int32_t temp_milli = ((int32_t)raw16 * 625) / 10;
    *raw = temp_milli;
    return 0;
}