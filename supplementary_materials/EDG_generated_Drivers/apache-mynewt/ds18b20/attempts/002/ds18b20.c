#include "ds18b20.h"
#include <errno.h>
#include <stdint.h>

#include <hal/hal_gpio.h>
#include "apache_mynewt.h"
#include <os/os_time.h>
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_SKIP_ROM 0xCC

#define DS18B20_RESET_LOW_US 480
#define DS18B20_PRESENCE_WAIT_US 480
#define DS18B20_CONVERSION_DELAY_MS 750

static int ds18b20_reset_pulse(struct ds18b20_dev *dev)
{
    hal_gpio_write(dev->data_pin, 0);
    os_cputime_delay_usecs(DS18B20_RESET_LOW_US);
    hal_gpio_write(dev->data_pin, 1);
    os_cputime_delay_usecs(DS18B20_PRESENCE_WAIT_US);
    int presence = hal_gpio_read(dev->data_pin);
    if (presence != 0) {
        return -ENODEV;
    }
    return 0;
}

static void ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        if (byte & 0x01) {
            hal_gpio_write(dev->data_pin, 0);
            os_cputime_delay_usecs(1);
            hal_gpio_write(dev->data_pin, 1);
            os_cputime_delay_usecs(60);
        } else {
            hal_gpio_write(dev->data_pin, 0);
            os_cputime_delay_usecs(60);
            hal_gpio_write(dev->data_pin, 1);
            os_cputime_delay_usecs(1);
        }
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(struct ds18b20_dev *dev)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        hal_gpio_write(dev->data_pin, 0);
        os_cputime_delay_usecs(1);
        hal_gpio_write(dev->data_pin, 1);
        os_cputime_delay_usecs(1);
        if (hal_gpio_read(dev->data_pin)) {
            byte |= 0x80;
        }
        os_cputime_delay_usecs(60);
    }
    return byte;
}

int ds18b20_init(struct ds18b20_dev *dev, int pin)
{
    dev->data_pin = pin;
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE);
    hal_gpio_write(pin, 1);
    int ret = ds18b20_reset_pulse(dev);
    if (ret != 0) {
        return ret;
    }
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    int ret = ds18b20_reset_pulse(dev);
    if (ret != 0) {
        return ret;
    }
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    os_time_delay(DS18B20_CONVERSION_DELAY_MS);
    ret = ds18b20_reset_pulse(dev);
    if (ret != 0) {
        return ret;
    }
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}
