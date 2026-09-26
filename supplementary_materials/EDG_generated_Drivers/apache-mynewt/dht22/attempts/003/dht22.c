#include "dht22.h"
#include <errno.h>
#include <stdint.h>

#include <hal/hal_gpio.h>
#include "apache_mynewt.h"
#include <os/os_time.h>
#define DHT22_TIMEOUT_US 200

static int dht22_wait_for_level(struct dht22_dev *dev, int level, uint32_t timeout_us)
{
    uint32_t wait = 0;
    while (hal_gpio_read(dev->data_pin) != level) {
        if (wait >= timeout_us) return -1;
        os_cputime_delay_usecs(1);
        wait++;
    }
    return 0;
}

int dht22_init(struct dht22_dev *dev, int data_pin)
{
    dev->data_pin = data_pin;
    hal_gpio_init_in(dev->data_pin, HAL_GPIO_PULL_NONE);
    os_time_delay(OS_TICKS_PER_SEC);
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity, int32_t *temperature)
{
    uint8_t data[5] = {0};
    int bit, i;
    uint32_t pulse_width;

    // Send start signal: pull low for at least 18ms
    hal_gpio_init_in(dev->data_pin, HAL_GPIO_PULL_NONE);
    hal_gpio_write(dev->data_pin, 0);
    hal_gpio_init_in(dev->data_pin, HAL_GPIO_PULL_NONE);
    hal_gpio_write(dev->data_pin, 0);
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 18);

    // Pull up and wait for sensor response
    hal_gpio_init_in(dev->data_pin, HAL_GPIO_PULL_NONE);
    os_cputime_delay_usecs(30);

    // Wait for sensor to pull low (response signal)
    if (dht22_wait_for_level(dev, 0, DHT22_TIMEOUT_US) != 0) {
        return -EIO;
    }
    // Wait for sensor to pull high
    if (dht22_wait_for_level(dev, 1, DHT22_TIMEOUT_US) != 0) {
        return -EIO;
    }

    // Read 40 bits
    for (i = 0; i < 40; i++) {
        // Wait for low (start of bit)
        if (dht22_wait_for_level(dev, 0, DHT22_TIMEOUT_US) != 0) {
            return -EIO;
        }
        // Wait for high
        if (dht22_wait_for_level(dev, 1, DHT22_TIMEOUT_US) != 0) {
            return -EIO;
        }
        // Measure high pulse width
        pulse_width = 0;
        while (hal_gpio_read(dev->data_pin) == 1) {
            if (pulse_width > 100) break;
            os_cputime_delay_usecs(1);
            pulse_width++;
        }
        // Determine bit: if pulse > 40us, it's 1
        if (pulse_width > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    // Extract integral and decimal parts
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    // Convert to milli units using integer approximation
    *humidity = (int32_t)raw_hum * 100 + (int32_t)raw_hum;
    *temperature = (int32_t)raw_temp * 100 + (int32_t)raw_temp;

    return 0;
}
