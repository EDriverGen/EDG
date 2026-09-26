#include "dht22.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#define DHT22_GPIO_DIR_OUT 0
#define DHT22_GPIO_DIR_IN 1
#define DHT22_GPIO_HIGH 1
#define DHT22_GPIO_LOW 0

#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int32_t dht22_wait_for_level(struct dht22_device *dev, uint16_t expected_level, uint32_t timeout_us)
{
    uint32_t elapsed = 0;
    uint16_t val;
    while (elapsed < timeout_us) {
        if (GpioRead(dev->gpio_pin, &val) != HDF_SUCCESS) {
            return -1;
        }
        if (val == expected_level) {
            return 0;
        }
        OsalUDelay(1);
        elapsed++;
    }
    return -1;
}

int32_t dht22_init(struct dht22_device *dev, uint16_t gpio_pin)
{
    dev->gpio_pin = gpio_pin;
    GpioSetDir(gpio_pin, DHT22_GPIO_DIR_OUT);
    GpioWrite(gpio_pin, DHT22_GPIO_HIGH);
    OsalMDelay(1000);
    return 0;
}

int32_t dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    uint16_t val;
    uint8_t data[5] = {0};
    int32_t ret;

    // Send start signal: pull low for at least 18ms
    GpioSetDir(dev->gpio_pin, DHT22_GPIO_DIR_OUT);
    GpioWrite(dev->gpio_pin, DHT22_GPIO_LOW);
    OsalMDelay(DHT22_START_SIGNAL_LOW_MS);
    GpioWrite(dev->gpio_pin, DHT22_GPIO_HIGH);
    OsalUDelay(DHT22_RESPONSE_WAIT_US);

    // Switch to input
    GpioSetDir(dev->gpio_pin, DHT22_GPIO_DIR_IN);

    // Wait for sensor response: low for 80us
    if (dht22_wait_for_level(dev, DHT22_GPIO_LOW, DHT22_TIMEOUT_US) != 0) {
        return -1;
    }
    if (dht22_wait_for_level(dev, DHT22_GPIO_HIGH, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US) != 0) {
        return -1;
    }
    // Wait for high for 80us
    if (dht22_wait_for_level(dev, DHT22_GPIO_LOW, DHT22_RESPONSE_HIGH_US + DHT22_TIMEOUT_US) != 0) {
        return -1;
    }

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        // Wait for low (start of bit)
        if (dht22_wait_for_level(dev, DHT22_GPIO_HIGH, DHT22_BIT_LOW_US + DHT22_TIMEOUT_US) != 0) {
            return -1;
        }
        // Wait for high
        if (dht22_wait_for_level(dev, DHT22_GPIO_LOW, DHT22_TIMEOUT_US) != 0) {
            return -1;
        }
        // Measure high pulse width
        uint32_t high_start = 0;
        uint32_t high_count = 0;
        while (1) {
            if (GpioRead(dev->gpio_pin, &val) != HDF_SUCCESS) {
                return -1;
            }
            if (val == DHT22_GPIO_LOW) {
                break;
            }
            OsalUDelay(1);
            high_count++;
            if (high_count > DHT22_TIMEOUT_US) {
                return -1;
            }
        }
        uint8_t bit = (high_count > (DHT22_BIT_0_HIGH_US + DHT22_BIT_1_HIGH_US) / 2) ? 1 : 0;
        data[i / 8] = (data[i / 8] << 1) | bit;
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -1;
    }

    // Convert humidity: integral_RH = data[0], decimal_RH = data[1]
    int32_t integral_rh = data[0];
    int32_t decimal_rh = data[1];
    *humidity_val = integral_rh * 100 + decimal_rh;

    // Convert temperature: integral_T = data[2], decimal_T = data[3]
    int32_t integral_t = data[2];
    int32_t decimal_t = data[3];
    *temp_val = integral_t * 100 + decimal_t;

    return 0;
}