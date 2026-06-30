/*
 * DHT22 sensor driver for RIOT (GPIO)
 */
#include "dht22_ref.h"

static int dht22_wait(gpio_t pin, int val, int max_us)
{
    int us = 0;
    while (gpio_read(pin) == val) {
        xtimer_usleep(1);
        if (++us > max_us) return -1;
    }
    return us;
}

int dht22_init(struct dht22_device *dev, gpio_t pin)
{
    if (!dev) return -1;
    dev->data_pin = pin;
    gpio_init(pin, GPIO_OUT);
    gpio_set(pin);
    return 0;
}

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (!dev || !temp_x10 || !humidity_x10) return -1;

    gpio_init(dev->data_pin, GPIO_OUT);
    gpio_clear(dev->data_pin);
    xtimer_msleep(2);
    gpio_set(dev->data_pin);
    xtimer_usleep(30);
    gpio_init(dev->data_pin, GPIO_IN);

    if (dht22_wait(dev->data_pin, 0, DHT22_TIMEOUT_US) < 0) return -1;
    if (dht22_wait(dev->data_pin, 1, DHT22_TIMEOUT_US) < 0) return -1;

    for (int i = 0; i < 40; i++) {
        if (dht22_wait(dev->data_pin, 0, DHT22_TIMEOUT_US) < 0) return -1;
        int high = dht22_wait(dev->data_pin, 1, DHT22_TIMEOUT_US);
        if (high < 0) return -1;
        data[i/8] <<= 1;
        if (high > DHT22_BIT_THRESHOLD) data[i/8] |= 1;
    }

    /* verify checksum */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return -1;
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
