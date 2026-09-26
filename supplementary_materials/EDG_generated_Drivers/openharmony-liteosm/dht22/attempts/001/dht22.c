#include "dht22.h"
#include "gpio_if.h"
#include "osal_time.h"
#include "hdf_base.h"
#include <stdint.h>

#define DHT22_GPIO_DIR_OUT 0
#define DHT22_GPIO_DIR_IN 1
#define DHT22_GPIO_VAL_LOW 0
#define DHT22_GPIO_VAL_HIGH 1

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
            return HDF_FAILURE;
        }
        if (val == expected_level) {
            return HDF_SUCCESS;
        }
        OsalUDelay(1);
        elapsed++;
    }
    return HDF_FAILURE;
}

int32_t dht22_init(struct dht22_device *dev, uint16_t gpio_pin)
{
    dev->gpio_pin = gpio_pin;
    OsalMDelay(1000);
    return HDF_SUCCESS;
}

int32_t dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    uint16_t val;
    uint8_t data[5] = {0};
    int32_t ret;

    // Set pin as output and pull low for start signal
    if (GpioSetDir(dev->gpio_pin, DHT22_GPIO_DIR_OUT) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    if (GpioWrite(dev->gpio_pin, DHT22_GPIO_VAL_LOW) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    OsalMDelay(DHT22_START_SIGNAL_LOW_MS);

    // Set pin as input to release bus
    if (GpioSetDir(dev->gpio_pin, DHT22_GPIO_DIR_IN) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    // Wait for sensor response (pull low)
    OsalUDelay(DHT22_RESPONSE_WAIT_US);
    if (dht22_wait_for_level(dev, DHT22_GPIO_VAL_LOW, DHT22_TIMEOUT_US) != HDF_SUCCESS) {
        return -EIO;
    }
    // Wait for response low duration
    if (dht22_wait_for_level(dev, DHT22_GPIO_VAL_HIGH, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US) != HDF_SUCCESS) {
        return -EIO;
    }
    // Wait for response high duration
    if (dht22_wait_for_level(dev, DHT22_GPIO_VAL_LOW, DHT22_RESPONSE_HIGH_US + DHT22_TIMEOUT_US) != HDF_SUCCESS) {
        return -EIO;
    }

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        // Wait for bit low (50us)
        if (dht22_wait_for_level(dev, DHT22_GPIO_VAL_HIGH, DHT22_BIT_LOW_US + DHT22_TIMEOUT_US) != HDF_SUCCESS) {
            return -EIO;
        }
        // Measure high pulse width
        uint32_t high_start = 0;
        uint16_t level;
        while (1) {
            if (GpioRead(dev->gpio_pin, &level) != HDF_SUCCESS) {
                return HDF_FAILURE;
            }
            if (level == DHT22_GPIO_VAL_LOW) {
                break;
            }
            OsalUDelay(1);
            high_start++;
            if (high_start > DHT22_TIMEOUT_US) {
                return -EIO;
            }
        }
        uint8_t bit = (high_start > (DHT22_BIT_0_HIGH_US + DHT22_BIT_1_HIGH_US) / 2) ? 1 : 0;
        data[i / 8] = (data[i / 8] << 1) | bit;
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    // Convert to milli units
    uint16_t integral_rh = data[0];
    uint16_t decimal_rh = data[1];
    uint16_t integral_t = data[2];
    uint16_t decimal_t = data[3];

    *humidity_val = (int32_t)integral_rh * 100 + (int32_t)decimal_rh;
    *temp_val = (int32_t)integral_t * 100 + (int32_t)decimal_t;

    return HDF_SUCCESS;
}