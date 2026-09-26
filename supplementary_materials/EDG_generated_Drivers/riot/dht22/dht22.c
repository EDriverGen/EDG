#include "dht22.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include "xtimer.h"

#include <periph/gpio.h>
#define DHT22_TIMEOUT_US 200

static int wait_for_level(struct dht22_device *dev, bool level, uint32_t timeout_us)
{
    uint32_t wait = 0;
    while (gpio_read(dev->pin) != level) {
        if (wait >= timeout_us) {
            return -ETIMEDOUT;
        }
        xtimer_usleep(1);
        wait++;
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
    if (!dev || !humidity_val || !temp_val) {
        return -EINVAL;
    }

    gpio_init(dev->pin, GPIO_OUT);
    gpio_set(dev->pin);
    xtimer_usleep(2);
    gpio_init(dev->pin, GPIO_OUT);
    gpio_clear(dev->pin);
    xtimer_msleep(18);
    gpio_init(dev->pin, GPIO_IN);
    gpio_set(dev->pin);
    xtimer_usleep(30);

    if (wait_for_level(dev, false, DHT22_TIMEOUT_US) != 0) {
        return -EIO;
    }
    if (wait_for_level(dev, true, DHT22_TIMEOUT_US) != 0) {
        return -EIO;
    }
    if (wait_for_level(dev, false, DHT22_TIMEOUT_US) != 0) {
        return -EIO;
    }

    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        if (wait_for_level(dev, true, DHT22_TIMEOUT_US) != 0) {
            return -EIO;
        }
        uint32_t high_start = 0;
        while (gpio_read(dev->pin)) {
            high_start++;
            xtimer_usleep(1);
            if (high_start > 100) {
                return -EIO;
            }
        }
        if (high_start > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if ((sum & 0xFF) != data[4]) {
        return -EIO;
    }

    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    int32_t temp_x10 = (raw_temp & 0x8000)
        ? -(int32_t)(raw_temp & 0x7FFF)
        : (int32_t)raw_temp;

    *humidity_val = (int32_t)raw_hum * 100;
    *temp_val = temp_x10 * 100;

    return 0;
}
