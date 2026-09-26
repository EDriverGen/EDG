#include "dht22.h"
#include <errno.h>
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int dht22_wait_for_level(const struct gpio_dt_spec *gpio, int expected, uint32_t timeout_us)
{
    uint32_t elapsed = 0;
    while (elapsed < timeout_us) {
        int val = gpio_pin_get_dt(gpio);
        if (val < 0) return val;
        if (val == expected) return 0;
        k_busy_wait(1);
        elapsed++;
    }
    return -EIO;
}

int dht22_init(struct dht22_data *dev, const struct device *gpio_dev)
{
    if (!dev || !gpio_dev) return -EINVAL;
    dev->gpio_dev = gpio_dev;
    dev->gpio.port = gpio_dev;
    dev->gpio.pin = 0;
    dev->gpio.dt_flags = GPIO_OUTPUT_LOW;
    int ret = gpio_pin_configure_dt(&dev->gpio, GPIO_OUTPUT_LOW);
    if (ret < 0) return ret;
    k_msleep(1000);
    return 0;
}

int dht22_read(struct dht22_data *dev, int32_t *humidity_val, int32_t *temp_val)
{
    if (!dev || !humidity_val || !temp_val) return -EINVAL;
    struct gpio_dt_spec *gpio = &dev->gpio;
    int ret;

    // Send start signal: pull low for at least 18ms
    ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_LOW);
    if (ret < 0) return ret;
    gpio_pin_set_dt(gpio, 0);
    k_msleep(DHT22_START_SIGNAL_LOW_MS);
    // Release line (input mode)
    ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
    if (ret < 0) return ret;
    // Wait for sensor response: pull low
    k_busy_wait(DHT22_RESPONSE_WAIT_US);
    ret = dht22_wait_for_level(gpio, 0, DHT22_TIMEOUT_US);
    if (ret < 0) return ret;
    // Wait for response low duration
    ret = dht22_wait_for_level(gpio, 1, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US);
    if (ret < 0) return ret;
    // Wait for response high duration
    ret = dht22_wait_for_level(gpio, 0, DHT22_RESPONSE_HIGH_US + DHT22_TIMEOUT_US);
    if (ret < 0) return ret;

    // Read 40 bits
    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        // Wait for bit low (50us)
        ret = dht22_wait_for_level(gpio, 1, DHT22_BIT_LOW_US + DHT22_TIMEOUT_US);
        if (ret < 0) return ret;
        // Measure high pulse width
        uint32_t high_start = 0;
        while (gpio_pin_get_dt(gpio) == 1) {
            k_busy_wait(1);
            high_start++;
            if (high_start > DHT22_TIMEOUT_US) return -EIO;
        }
        // Classify bit
        if (high_start > DHT22_BIT_0_HIGH_US + 5) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    // Convert to milli units
    uint16_t integral_rh = data[0];
    uint16_t decimal_rh = data[1];
    uint16_t integral_t = data[2];
    uint16_t decimal_t = data[3];

    *humidity_val = (int32_t)(integral_rh * 100 + decimal_rh);
    *temp_val = (int32_t)(integral_t * 100 + decimal_t);

    return 0;
}