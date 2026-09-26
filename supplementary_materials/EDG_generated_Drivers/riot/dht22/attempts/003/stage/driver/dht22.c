#include "dht22.h"
#include "xtimer.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <periph/gpio.h>

#define DHT22_TIMEOUT_US 200

static int dht22_wait_for_level(gpio_t pin, bool level, uint32_t timeout_us)
{
    uint32_t elapsed = 0;
    while (gpio_read(pin) != level) {
        if (elapsed >= timeout_us) {
            return -EIO;
        }
        xtimer_usleep(1);
        elapsed++;
    }
    return 0;
}

int dht22_init(struct dht22_device *dev, gpio_t pin)
{
    dev->pin = pin;
    gpio_init(pin, GPIO_OUT);
    gpio_set(pin);
    xtimer_msleep(1000);
    return 0;
}

int dht22_read(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    gpio_t pin = dev->pin;
    uint8_t data[5] = {0};
    int ret;

    // Send start signal: pull low for at least 18ms
    gpio_init(pin, GPIO_OUT);
    gpio_set(pin);
    xtimer_usleep(1);
    gpio_init(pin, GPIO_OUT);
    gpio_write(pin, 0);
    xtimer_msleep(20);
    gpio_init(pin, GPIO_IN);
    gpio_set(pin);

    // Wait for sensor response: low for 80us, then high for 80us
    ret = dht22_wait_for_level(pin, false, DHT22_TIMEOUT_US);
    if (ret < 0) return ret;
    ret = dht22_wait_for_level(pin, true, DHT22_TIMEOUT_US);
    if (ret < 0) return ret;
    ret = dht22_wait_for_level(pin, false, DHT22_TIMEOUT_US);
    if (ret < 0) return ret;

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        ret = dht22_wait_for_level(pin, true, DHT22_TIMEOUT_US);
        if (ret < 0) return ret;
        uint32_t high_start = 0;
        while (gpio_read(pin)) {
            high_start++;
            xtimer_usleep(1);
            if (high_start > 100) return -EIO;
        }
        uint8_t bit = (high_start > 40) ? 1 : 0;
        data[i / 8] = (data[i / 8] << 1) | bit;
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    // Convert to milli units
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    *humidity_val = (int32_t)raw_hum * 100 + (int32_t)raw_hum;
    *temp_val = (int32_t)raw_temp * 100 + (int32_t)raw_temp;

    return 0;
}