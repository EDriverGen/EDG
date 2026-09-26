#include "dht22.h"
#include "rtthread.h"
#include <stdint.h>
#include <errno.h>

#define DHT22_PIN_MODE_OUTPUT 0
#define DHT22_PIN_MODE_INPUT 1
#define DHT22_PIN_HIGH 1
#define DHT22_PIN_LOW 0

#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static void delay_us(volatile uint32_t us) {
    volatile uint32_t count;
    for (; us > 0; us--) {
        for (count = 0; count < 10; count++) {
            __asm__ volatile ("nop");
        }
    }
}

int dht22_init(struct dht22_device *dev, uint64_t pin) {
    dev->pin = pin;
    rt_pin_mode(pin, DHT22_PIN_MODE_OUTPUT);
    rt_pin_write(pin, DHT22_PIN_HIGH);
    rt_thread_mdelay(1000);
    return 0;
}

static int dht22_wait_for_pin_state(struct dht22_device *dev, rt_int8_t state, uint32_t timeout_us) {
    uint32_t count = 0;
    while (rt_pin_read(dev->pin) != state) {
        delay_us(1);
        count++;
        if (count > timeout_us) {
            return -ETIMEDOUT;
        }
    }
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature) {
    uint8_t data[5] = {0};
    int ret;

    if (!humidity || !temperature) {
        return -EINVAL;
    }

    // Send start signal: pull low for at least 18ms
    rt_pin_mode(dev->pin, DHT22_PIN_MODE_OUTPUT);
    rt_pin_write(dev->pin, DHT22_PIN_LOW);
    rt_thread_mdelay(DHT22_START_SIGNAL_LOW_MS);
    rt_pin_write(dev->pin, DHT22_PIN_HIGH);
    delay_us(DHT22_RESPONSE_WAIT_US);

    // Switch to input
    rt_pin_mode(dev->pin, DHT22_PIN_MODE_INPUT);

    // Wait for sensor response: low for 80us
    ret = dht22_wait_for_pin_state(dev, DHT22_PIN_LOW, DHT22_TIMEOUT_US);
    if (ret != 0) {
        return -EIO;
    }
    delay_us(DHT22_RESPONSE_LOW_US);

    // Wait for sensor response: high for 80us
    ret = dht22_wait_for_pin_state(dev, DHT22_PIN_HIGH, DHT22_TIMEOUT_US);
    if (ret != 0) {
        return -EIO;
    }
    delay_us(DHT22_RESPONSE_HIGH_US);

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        // Wait for low (50us)
        ret = dht22_wait_for_pin_state(dev, DHT22_PIN_LOW, DHT22_TIMEOUT_US);
        if (ret != 0) {
            return -EIO;
        }
        delay_us(DHT22_BIT_LOW_US);

        // Wait for high and measure its duration
        uint32_t high_start = 0;
        while (rt_pin_read(dev->pin) == DHT22_PIN_LOW) {
            delay_us(1);
            high_start++;
            if (high_start > DHT22_TIMEOUT_US) {
                return -EIO;
            }
        }
        uint32_t high_count = 0;
        while (rt_pin_read(dev->pin) == DHT22_PIN_HIGH) {
            delay_us(1);
            high_count++;
            if (high_count > DHT22_TIMEOUT_US) {
                return -EIO;
            }
        }

        // Classify bit: if high pulse > 40us (midpoint between 27 and 70), it's 1
        if (high_count > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    // Extract integral and decimal parts
    uint16_t integral_RH = data[0];
    uint16_t decimal_RH = data[1];
    uint16_t integral_T = data[2];
    uint16_t decimal_T = data[3];

    // Convert to milli units using integer approximation
    // humidity = integral_RH * 100 + decimal_RH
    // temperature = integral_T * 100 + decimal_T
    *humidity = (int32_t)integral_RH * 100 + (int32_t)decimal_RH;
    *temperature = (int32_t)integral_T * 100 + (int32_t)decimal_T;

    return 0;
}