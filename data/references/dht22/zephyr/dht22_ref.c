/*
 * DHT22 sensor driver for Zephyr (GPIO)
 */
#include "dht22_ref.h"

static int dht22_wait(const struct gpio_dt_spec *pin, int val, int max_us)
{
    int us = 0;
    while (gpio_pin_get_dt(pin) == val) {
        k_busy_wait(1);
        if (++us > max_us) return -1;
    }
    return us;
}

int dht22_init(struct dht22_device *dev, const struct gpio_dt_spec *data)
{
    if (!dev || !data) return -EINVAL;
    dev->data = data;
    gpio_pin_configure_dt(data, GPIO_OUTPUT_HIGH);
    return 0;
}

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (!dev || !dev->data || !temp_x10 || !humidity_x10) return -EINVAL;

    /* start signal */
    gpio_pin_configure_dt(dev->data, GPIO_OUTPUT_LOW);
    k_msleep(2);
    gpio_pin_set_dt(dev->data, 1);
    k_busy_wait(30);
    gpio_pin_configure_dt(dev->data, GPIO_INPUT);

    /* sensor response */
    if (dht22_wait(dev->data, 0, DHT22_TIMEOUT_US) < 0) return -EIO;
    if (dht22_wait(dev->data, 1, DHT22_TIMEOUT_US) < 0) return -EIO;

    /* read 40 bits */
    for (int i = 0; i < 40; i++) {
        if (dht22_wait(dev->data, 0, DHT22_TIMEOUT_US) < 0) return -EIO;
        int high = dht22_wait(dev->data, 1, DHT22_TIMEOUT_US);
        if (high < 0) return -EIO;
        data[i / 8] <<= 1;
        if (high > DHT22_BIT_THRESHOLD) data[i / 8] |= 1;
    }

    /* verify checksum */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return -EIO;
    /* humidity: data[0..1], 0.1% RH */
    *humidity_x10 = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    /* temperature: data[2..3], 0.1 degC, bit15=sign */
    uint16_t raw_t = (uint16_t)((uint16_t)data[2] << 8 | data[3]);
    if (raw_t & 0x8000)
        *temp_x10 = -(int16_t)(raw_t & 0x7FFF);
    else
        *temp_x10 = (int16_t)raw_t;
    return 0;
}
