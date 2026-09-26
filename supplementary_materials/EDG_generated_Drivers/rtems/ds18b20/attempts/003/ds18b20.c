#include "ds18b20.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <rtems/gpio.h>
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

#define DS18B20_RESET_LOW_US 480
#define DS18B20_RESET_HIGH_US 480
#define DS18B20_PRESENCE_WAIT_US 60
#define DS18B20_PRESENCE_LOW_US 240
#define DS18B20_CONVERSION_MS 750
#define DS18B20_SLOT_US 120
#define DS18B20_RECOVERY_US 1
#define DS18B20_WRITE0_LOW_US 120
#define DS18B20_WRITE1_LOW_US 15
#define DS18B20_READ_VALID_US 15

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 10; j++) {}
    }
}

static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}

static int ds18b20_reset(struct ds18b20_dev *dev) {
    rtems_gpio_set(dev->data_pin);
    delay_us(DS18B20_RESET_HIGH_US);
    rtems_gpio_set(dev->data_pin);
    delay_us(DS18B20_RESET_LOW_US);
    rtems_gpio_set(dev->data_pin);
    delay_us(DS18B20_PRESENCE_WAIT_US);
    int val = rtems_gpio_get_value(dev->data_pin);
    if (val != 0) {
        return -ENODEV;
    }
    delay_us(DS18B20_PRESENCE_LOW_US);
    return 0;
}

static void ds18b20_write_bit(struct ds18b20_dev *dev, int bit) {
    rtems_gpio_set(dev->data_pin);
    if (bit) {
        delay_us(DS18B20_WRITE1_LOW_US);
        rtems_gpio_set(dev->data_pin);
        delay_us(DS18B20_SLOT_US - DS18B20_WRITE1_LOW_US);
    } else {
        delay_us(DS18B20_WRITE0_LOW_US);
        rtems_gpio_set(dev->data_pin);
        delay_us(DS18B20_SLOT_US - DS18B20_WRITE0_LOW_US);
    }
    delay_us(DS18B20_RECOVERY_US);
}

static int ds18b20_read_bit(struct ds18b20_dev *dev) {
    rtems_gpio_set(dev->data_pin);
    delay_us(DS18B20_READ_VALID_US);
    int val = rtems_gpio_get_value(dev->data_pin);
    delay_us(DS18B20_SLOT_US - DS18B20_READ_VALID_US);
    delay_us(DS18B20_RECOVERY_US);
    return val;
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
    rtems_gpio_request_pin(data_pin, RTEMS_GPIO_FUNCTION_NONE, true, false, NULL);
    rtems_gpio_set(data_pin);
    delay_us(DS18B20_RESET_HIGH_US);
    int ret = ds18b20_reset(dev);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    int ret;
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    delay_ms(DS18B20_CONVERSION_MS);
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw16 = (int16_t)((msb << 8) | lsb);
    int32_t temp = (int32_t)raw16 * 625 / 10;
    *raw = temp;
    return 0;
}
