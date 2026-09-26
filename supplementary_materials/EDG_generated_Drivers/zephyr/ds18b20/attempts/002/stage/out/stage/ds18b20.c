#include "ds18b20.h"
#include <errno.h>
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

#define RESET_PULSE_US 480
#define PRESENCE_WAIT_US 480
#define CONVERSION_DELAY_MS 750

static int ds18b20_reset_pulse(struct ds18b20_dev *dev)
{
    int ret;
    struct gpio_dt_spec spec = { .port = dev->port, .pin = dev->pin, .dt_flags = GPIO_OUTPUT_INACTIVE };

    ret = gpio_pin_configure_dt(&spec, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;

    ret = gpio_pin_set_dt(&spec, 0);
    if (ret < 0) return ret;

    k_busy_wait(RESET_PULSE_US);

    ret = gpio_pin_configure_dt(&spec, GPIO_INPUT);
    if (ret < 0) return ret;

    k_busy_wait(PRESENCE_WAIT_US);

    int presence = gpio_pin_get_dt(&spec);
    if (presence < 0) return presence;
    if (presence != 0) return -ENODEV;

    return 0;
}

static int ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    struct gpio_dt_spec spec = { .port = dev->port, .pin = dev->pin, .dt_flags = GPIO_OUTPUT_INACTIVE };
    int ret;

    for (int i = 0; i < 8; i++) {
        ret = gpio_pin_configure_dt(&spec, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) return ret;

        if (byte & (1 << i)) {
            gpio_pin_set_dt(&spec, 0);
            k_busy_wait(1);
            gpio_pin_set_dt(&spec, 1);
            k_busy_wait(60);
        } else {
            gpio_pin_set_dt(&spec, 0);
            k_busy_wait(60);
            gpio_pin_set_dt(&spec, 1);
            k_busy_wait(1);
        }
    }

    ret = gpio_pin_configure_dt(&spec, GPIO_INPUT);
    if (ret < 0) return ret;

    return 0;
}

static int ds18b20_read_byte(struct ds18b20_dev *dev, uint8_t *byte)
{
    struct gpio_dt_spec spec = { .port = dev->port, .pin = dev->pin, .dt_flags = GPIO_OUTPUT_INACTIVE };
    int ret;
    uint8_t val = 0;

    for (int i = 0; i < 8; i++) {
        ret = gpio_pin_configure_dt(&spec, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) return ret;

        gpio_pin_set_dt(&spec, 0);
        k_busy_wait(1);
        gpio_pin_set_dt(&spec, 1);
        k_busy_wait(1);

        ret = gpio_pin_configure_dt(&spec, GPIO_INPUT);
        if (ret < 0) return ret;

        int bit = gpio_pin_get_dt(&spec);
        if (bit < 0) return bit;
        if (bit) val |= (1 << i);

        k_busy_wait(60);
    }

    *byte = val;
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, const struct device *port, gpio_pin_t pin)
{
    dev->port = port;
    dev->pin = pin;

    int ret = ds18b20_reset_pulse(dev);
    if (ret < 0) return ret;

    ret = ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    if (ret < 0) return ret;

    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    int ret;
    uint8_t lsb, msb;

    ret = ds18b20_reset_pulse(dev);
    if (ret < 0) return ret;

    ret = ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    if (ret < 0) return ret;

    ret = ds18b20_write_byte(dev, DS18B20_CONVERT_T);
    if (ret < 0) return ret;

    k_msleep(CONVERSION_DELAY_MS);

    ret = ds18b20_reset_pulse(dev);
    if (ret < 0) return ret;

    ret = ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    if (ret < 0) return ret;

    ret = ds18b20_write_byte(dev, DS18B20_READ_SCRATCHPAD);
    if (ret < 0) return ret;

    ret = ds18b20_read_byte(dev, &lsb);
    if (ret < 0) return ret;

    ret = ds18b20_read_byte(dev, &msb);
    if (ret < 0) return ret;

    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;

    return 0;
}