#include "dht22.h"
#include "rtthread.h"
#include <stdint.h>
#include <errno.h>

#define DHT22_PIN_MODE_OUTPUT 0
#define DHT22_PIN_MODE_INPUT 1
#define DHT22_PIN_HIGH 1
#define DHT22_PIN_LOW 0

static void delay_us(uint32_t us)
{
    volatile uint32_t i;
    for (i = 0; i < us * 10; i++) {
        __asm__ volatile ("nop");
    }
}

int dht22_init(struct dht22_device *dev, uint64_t pin)
{
    dev->pin = pin;
    rt_pin_mode(pin, DHT22_PIN_MODE_OUTPUT);
    rt_pin_write(pin, DHT22_PIN_HIGH);
    rt_thread_mdelay(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature)
{
    uint64_t pin = dev->pin;
    uint8_t data[5] = {0};
    int bit_idx, byte_idx;
    uint32_t timeout;
    int val;

    /* Send start signal: pull low for at least 18ms */
    rt_pin_mode(pin, DHT22_PIN_MODE_OUTPUT);
    rt_pin_write(pin, DHT22_PIN_LOW);
    rt_thread_mdelay(20);
    rt_pin_write(pin, DHT22_PIN_HIGH);
    delay_us(30);

    /* Switch to input */
    rt_pin_mode(pin, DHT22_PIN_MODE_INPUT);

    /* Wait for sensor response: low for 80us */
    timeout = 1000;
    while (rt_pin_read(pin) == DHT22_PIN_HIGH && timeout--) {
        delay_us(1);
    }
    if (timeout == 0) {
        return -EIO;
    }
    timeout = 1000;
    while (rt_pin_read(pin) == DHT22_PIN_LOW && timeout--) {
        delay_us(1);
    }
    if (timeout == 0) {
        return -EIO;
    }
    /* Wait for high for 80us */
    timeout = 1000;
    while (rt_pin_read(pin) == DHT22_PIN_HIGH && timeout--) {
        delay_us(1);
    }
    if (timeout == 0) {
        return -EIO;
    }

    /* Read 40 bits */
    for (byte_idx = 0; byte_idx < 5; byte_idx++) {
        for (bit_idx = 7; bit_idx >= 0; bit_idx--) {
            /* Wait for low (50us) */
            timeout = 1000;
            while (rt_pin_read(pin) == DHT22_PIN_HIGH && timeout--) {
                delay_us(1);
            }
            if (timeout == 0) {
                return -EIO;
            }
            /* Wait for high */
            timeout = 1000;
            while (rt_pin_read(pin) == DHT22_PIN_LOW && timeout--) {
                delay_us(1);
            }
            if (timeout == 0) {
                return -EIO;
            }
            /* Measure high pulse width */
            uint32_t cnt = 0;
            while (rt_pin_read(pin) == DHT22_PIN_HIGH && cnt < 1000) {
                cnt++;
                delay_us(1);
            }
            if (cnt > 30) {
                data[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    /* Verify checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    /* Extract values */
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    /* Convert to milli units: raw * 100 + raw = raw * 101? Wait, formula: integral_RH * 100 + decimal_RH, but raw_hum = integral_RH*256 + decimal_RH? Actually DHT22 data: integral_RH is data[0], decimal_RH is data[1]. So integral_RH = data[0], decimal_RH = data[1]. So humidity = data[0] * 100 + data[1]. Similarly temperature = data[2] * 100 + data[3]. */
    *humidity = (int32_t)data[0] * 100 + (int32_t)data[1];
    *temperature = (int32_t)data[2] * 100 + (int32_t)data[3];

    return 0;
}