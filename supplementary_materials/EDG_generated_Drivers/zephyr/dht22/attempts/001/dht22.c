#include "dht22.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
#include <stdint.h>

#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int dht22_wait_for_level(const struct gpio_dt_spec *spec, int expected_level, uint32_t timeout_us)
{
    uint32_t elapsed = 0;
    while (elapsed < timeout_us) {
        int val = gpio_pin_get_dt(spec);
        if (val < 0) return val;
        if (val == expected_level) return 0;
        k_busy_wait(1);
        elapsed++;
    }
    return -ETIMEDOUT;
}

int dht22_init(struct dht22_data *dev, const struct device *gpio_dev)
{
    if (!dev || !gpio_dev) return -EINVAL;
    dev->gpio_dev = gpio_dev;
    dev->gpio_spec.port = gpio_dev;
    dev->gpio_spec.pin = 5; /* GPIOB pin 5 — must match Renode slave */
    dev->gpio_spec.dt_flags = GPIO_OUTPUT_LOW;
    /* Wait power-up delay */
    k_msleep(1000);
    return 0;
}

int dht22_read(struct dht22_data *dev, int32_t *humidity_val, int32_t *temp_val)
{
    if (!dev || !humidity_val || !temp_val) return -EINVAL;
    const struct gpio_dt_spec *spec = &dev->gpio_spec;
    int ret;
    uint8_t data[5] = {0};
    int bit_idx;

    /* Configure as output low */
    ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT_LOW);
    if (ret < 0) return ret;

    /* Send start signal: pull low for at least 18ms */
    ret = gpio_pin_set_dt(spec, 0);
    if (ret < 0) return ret;
    k_msleep(DHT22_START_SIGNAL_LOW_MS);

    /* Pull high and release (switch to input) */
    ret = gpio_pin_set_dt(spec, 1);
    if (ret < 0) return ret;
    k_busy_wait(DHT22_RESPONSE_WAIT_US);

    /* Configure as input */
    ret = gpio_pin_configure_dt(spec, GPIO_INPUT);
    if (ret < 0) return ret;

    /* Wait for sensor response: low for 80us */
    ret = dht22_wait_for_level(spec, 0, DHT22_TIMEOUT_US);
    if (ret < 0) return -EIO;
    ret = dht22_wait_for_level(spec, 1, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US);
    if (ret < 0) return -EIO;

    /* Wait for high for 80us */
    ret = dht22_wait_for_level(spec, 0, DHT22_RESPONSE_HIGH_US + DHT22_TIMEOUT_US);
    if (ret < 0) return -EIO;

    /* Read 40 bits */
    for (bit_idx = 0; bit_idx < 40; bit_idx++) {
        /* Wait for low (50us) */
        ret = dht22_wait_for_level(spec, 0, DHT22_TIMEOUT_US);
        if (ret < 0) return -EIO;
        /* Wait for high */
        ret = dht22_wait_for_level(spec, 1, DHT22_TIMEOUT_US);
        if (ret < 0) return -EIO;
        /* Measure high pulse width */
        uint32_t high_start = 0;
        while (1) {
            int val = gpio_pin_get_dt(spec);
            if (val < 0) return val;
            if (val == 0) break;
            high_start++;
            k_busy_wait(1);
            if (high_start > DHT22_TIMEOUT_US) return -EIO;
        }
        /* Classify bit */
        if (high_start > DHT22_BIT_0_HIGH_US + 5) {
            data[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        }
    }

    /* Verify checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    /* Convert to milli units */
    uint16_t integral_rh = ((uint16_t)data[0] << 8) | data[1];
    uint16_t integral_t = ((uint16_t)data[2] << 8) | data[3];
    *humidity_val = (int32_t)integral_rh * 100 + (int32_t)data[1];
    *temp_val = (int32_t)integral_t * 100 + (int32_t)data[3];

    return 0;
}