#include "dht22.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include <rtems/gpio.h>
#define DHT22_START_SIGNAL_LOW_US 18000
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int dht22_wait_for_level(uint32_t pin, int expected_level, int timeout_us) {
    int level;
    for (int i = 0; i < timeout_us; i++) {
        level = rtems_gpio_get_value(pin);
        if (level == expected_level) {
            return 0;
        }
        usleep(1);
    }
    return -ETIMEDOUT;
}

int dht22_init(struct dht22_device *dev, uint32_t data_pin) {
    dev->data_pin = data_pin;
    rtems_gpio_request_pin(data_pin, RTEMS_GPIO_PIN_OUTPUT, true, false, NULL);
    rtems_gpio_set(data_pin);
    sleep(1);
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    uint32_t pin = dev->data_pin;
    uint8_t data[5] = {0};
    int ret;

    rtems_gpio_request_pin(pin, RTEMS_GPIO_PIN_OUTPUT, true, false, NULL);
    rtems_gpio_set(pin);
    usleep(1);
    rtems_gpio_set(pin);
    usleep(DHT22_START_SIGNAL_LOW_US);

    rtems_gpio_request_pin(pin, RTEMS_GPIO_PIN_INPUT, false, false, NULL);

    ret = dht22_wait_for_level(pin, 0, DHT22_TIMEOUT_US);
    if (ret != 0) return -EIO;
    ret = dht22_wait_for_level(pin, 1, DHT22_TIMEOUT_US);
    if (ret != 0) return -EIO;

    for (int i = 0; i < 40; i++) {
        ret = dht22_wait_for_level(pin, 0, DHT22_TIMEOUT_US);
        if (ret != 0) return -EIO;
        ret = dht22_wait_for_level(pin, 1, DHT22_TIMEOUT_US);
        if (ret != 0) return -EIO;
        int high_time = 0;
        while (rtems_gpio_get_value(pin) == 1 && high_time < DHT22_TIMEOUT_US) {
            high_time++;
            usleep(1);
        }
        if (high_time >= DHT22_TIMEOUT_US) return -EIO;
        int bit = (high_time > (DHT22_BIT_0_HIGH_US + DHT22_BIT_1_HIGH_US) / 2) ? 1 : 0;
        data[i / 8] = (data[i / 8] << 1) | bit;
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    int32_t integral_RH = data[0];
    int32_t decimal_RH = data[1];
    int32_t integral_T = data[2];
    int32_t decimal_T = data[3];

    *humidity_val = integral_RH * 100 + decimal_RH;
    *temp_val = integral_T * 100 + decimal_T;

    return 0;
}
