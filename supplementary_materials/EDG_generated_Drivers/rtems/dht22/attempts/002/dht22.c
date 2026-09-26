#include "dht22.h"
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include <rtems/gpio.h>
#define DHT22_DATA_PIN PB5

static int dht22_wait_for_level(uint32_t pin, int level, int timeout_us)
{
    int elapsed = 0;
    while (elapsed < timeout_us) {
        int val = rtems_gpio_get_value(pin);
        if (val == level) {
            return elapsed;
        }
        usleep(1);
        elapsed += 1;
    }
    return -1;
}

int dht22_init(struct dht22_device *dev, uint32_t data_pin)
{
    dev->data_pin = data_pin;
    rtems_gpio_request_pin(data_pin, RTEMS_GPIO_OUTPUT, true, false, NULL);
    rtems_gpio_set(data_pin);
    usleep(1000000);
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    uint32_t pin = dev->data_pin;
    uint8_t data[5] = {0};
    int ret;

    rtems_gpio_request_pin(pin, RTEMS_GPIO_OUTPUT, true, false, NULL);
    rtems_gpio_set(pin);
    usleep(20000);
    rtems_gpio_set(pin);
    usleep(40);

    rtems_gpio_request_pin(pin, RTEMS_GPIO_INPUT, false, false, NULL);

    ret = dht22_wait_for_level(pin, 0, 100);
    if (ret < 0) {
        return -EIO;
    }
    ret = dht22_wait_for_level(pin, 1, 200);
    if (ret < 0) {
        return -EIO;
    }
    ret = dht22_wait_for_level(pin, 0, 200);
    if (ret < 0) {
        return -EIO;
    }

    for (int i = 0; i < 40; i++) {
        ret = dht22_wait_for_level(pin, 1, 100);
        if (ret < 0) {
            return -EIO;
        }
        int high_time = 0;
        while (rtems_gpio_get_value(pin) == 1) {
            usleep(1);
            high_time++;
            if (high_time > 100) break;
        }
        if (high_time > 50) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    *humidity_val = (int32_t)raw_hum * 100 + (int32_t)raw_hum;
    *temp_val = (int32_t)raw_temp * 100 + (int32_t)raw_temp;

    return 0;
}
