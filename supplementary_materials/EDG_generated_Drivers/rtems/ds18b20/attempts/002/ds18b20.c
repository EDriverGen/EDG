#include "ds18b20.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <rtems/gpio.h>
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_SKIP_ROM 0xCC

#define DS18B20_RESET_LOW_US 480
#define DS18B20_RESET_HIGH_US 480
#define DS18B20_PRESENCE_WAIT_US 60
#define DS18B20_PRESENCE_TIMEOUT_US 240
#define DS18B20_CONVERSION_WAIT_MS 750
#define DS18B20_SLOT_DURATION_US 120
#define DS18B20_RECOVERY_US 1
#define DS18B20_READ_DATA_VALID_US 15

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        __asm__ volatile ("nop");
    }
}

static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}

static int gpio_set(uint32_t pin, bool value) {
    if (value) {
        return rtems_gpio_set(pin);
    } else {
        return rtems_gpio_clear(pin);
    }
}

static int gpio_get(uint32_t pin) {
    return rtems_gpio_get_value(pin);
}

static int gpio_output(uint32_t pin) {
    return rtems_gpio_request_pin(pin, RTEMS_GPIO_PIN_OUTPUT, true, false, NULL);
}

static int gpio_input(uint32_t pin) {
    return rtems_gpio_request_pin(pin, RTEMS_GPIO_PIN_INPUT, false, false, NULL);
}

static int ds18b20_reset(struct ds18b20_dev *dev) {
    int ret;
    ret = gpio_output(dev->data_pin);
    if (ret != 0) return ret;
    ret = gpio_set(dev->data_pin, 0);
    if (ret != 0) return ret;
    delay_us(DS18B20_RESET_LOW_US);
    ret = gpio_input(dev->data_pin);
    if (ret != 0) return ret;
    delay_us(DS18B20_PRESENCE_WAIT_US);
    int val = gpio_get(dev->data_pin);
    if (val != 0) {
        return -ENODEV;
    }
    delay_us(DS18B20_PRESENCE_TIMEOUT_US);
    val = gpio_get(dev->data_pin);
    if (val != 1) {
        return -ENODEV;
    }
    return 0;
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, int bit) {
    gpio_output(dev->data_pin);
    gpio_set(dev->data_pin, 0);
    if (bit) {
        delay_us(DS18B20_READ_DATA_VALID_US);
        gpio_set(dev->data_pin, 1);
        delay_us(DS18B20_SLOT_DURATION_US - DS18B20_READ_DATA_VALID_US);
    } else {
        delay_us(DS18B20_SLOT_DURATION_US);
        gpio_set(dev->data_pin, 1);
    }
    delay_us(DS18B20_RECOVERY_US);
}

static int ds18b20_read_bit(struct ds18b20_dev *dev) {
    int bit;
    gpio_output(dev->data_pin);
    gpio_set(dev->data_pin, 0);
    delay_us(DS18B20_READ_DATA_VALID_US);
    gpio_input(dev->data_pin);
    delay_us(DS18B20_READ_DATA_VALID_US);
    bit = gpio_get(dev->data_pin);
    delay_us(DS18B20_SLOT_DURATION_US - 2 * DS18B20_READ_DATA_VALID_US);
    delay_us(DS18B20_RECOVERY_US);
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

int ds18b20_init(struct ds18b20_dev *dev, uint32_t data_pin) {
    dev->data_pin = data_pin;
    return ds18b20_reset(dev);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    int ret;
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    delay_ms(DS18B20_CONVERSION_WAIT_MS);
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}
