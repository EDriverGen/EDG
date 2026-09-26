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

static void dht22_delay_us(uint32_t us) {
    (void)us;
}

static int dht22_wait_for_level(uint32_t pin, int expected_level, int timeout_us) {
    int level = 0;
    for (int i = 0; i < timeout_us; i++) {
        if (rtems_gpio_get_value(pin, &level) != RTEMS_SUCCESSFUL) {
            return -EIO;
        }
        if (level == expected_level) {
            return 0;
        }
        dht22_delay_us(1);
    }
    return -ETIMEDOUT;
}

int dht22_init(struct dht22_device *dev, uint32_t data_pin) {
    dev->data_pin = data_pin;
    rtems_gpio_request_pin(data_pin, RTEMS_GPIO_OUTPUT);
    rtems_gpio_set(data_pin);
    dht22_delay_us(1000000);
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    uint32_t pin = dev->data_pin;
    uint8_t data[5] = {0};
    int ret;

    rtems_gpio_request_pin(pin, RTEMS_GPIO_OUTPUT);
    rtems_gpio_clear(pin);
    dht22_delay_us(DHT22_START_SIGNAL_LOW_US);
    rtems_gpio_set(pin);
    dht22_delay_us(DHT22_RESPONSE_WAIT_US);

    rtems_gpio_request_pin(pin, RTEMS_GPIO_INPUT);

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
        int level = 0;
        while (high_time < DHT22_TIMEOUT_US) {
            if (rtems_gpio_get_value(pin, &level) != RTEMS_SUCCESSFUL) return -EIO;
            if (level != 1) break;
            high_time++;
            dht22_delay_us(1);
        }
        if (high_time >= DHT22_TIMEOUT_US) return -EIO;
        int bit = (high_time > (DHT22_BIT_0_HIGH_US + DHT22_BIT_1_HIGH_US) / 2) ? 1 : 0;
        data[i / 8] = (data[i / 8] << 1) | bit;
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    int32_t temp_x10 = (raw_temp & 0x8000)
        ? -(int32_t)(raw_temp & 0x7FFF)
        : (int32_t)raw_temp;

    *humidity_val = (int32_t)raw_hum * 100;
    *temp_val = temp_x10 * 100;

    return 0;
}
