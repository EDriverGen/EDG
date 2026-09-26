#include "dht22.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <rtems/gpio.h>
#include "rtems.h"
#define DHT22_DATA_PIN PB5

static void delay_us(uint32_t us) {
    usleep(us);
}

static void delay_ms(uint32_t ms) {
    usleep(ms * 1000);
}

int dht22_init(struct dht22_device *dev, uint32_t data_pin) {
    dev->data_pin = data_pin;
    rtems_status_code sc;
    sc = rtems_gpio_request_pin(data_pin, RTEMS_GPIO_PIN_OUTPUT, true, false, NULL);
    if (sc != RTEMS_SUCCESSFUL) {
        return -EIO;
    }
    rtems_gpio_set(data_pin);
    delay_ms(1000);
    return 0;
}

static int read_bit(struct dht22_device *dev) {
    uint32_t pin = dev->data_pin;
    int timeout = 100;
    while (rtems_gpio_get_value(pin) == 0) {
        if (--timeout == 0) return -1;
        delay_us(1);
    }
    delay_us(30);
    int val = rtems_gpio_get_value(pin);
    while (rtems_gpio_get_value(pin) == 1) {
        if (--timeout == 0) return -1;
        delay_us(1);
    }
    return val;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature) {
    uint32_t pin = dev->data_pin;
    uint8_t data[5] = {0};
    int i, j;

    rtems_gpio_request_pin(pin, RTEMS_GPIO_PIN_OUTPUT, true, false, NULL);
    rtems_gpio_set(pin);
    delay_ms(2);
    rtems_gpio_set(pin);
    delay_us(20);

    rtems_gpio_request_pin(pin, RTEMS_GPIO_PIN_INPUT, false, false, NULL);
    int timeout = 1000;
    while (rtems_gpio_get_value(pin) == 1) {
        if (--timeout == 0) return -EIO;
        delay_us(1);
    }
    timeout = 1000;
    while (rtems_gpio_get_value(pin) == 0) {
        if (--timeout == 0) return -EIO;
        delay_us(1);
    }
    timeout = 1000;
    while (rtems_gpio_get_value(pin) == 1) {
        if (--timeout == 0) return -EIO;
        delay_us(1);
    }

    for (i = 0; i < 5; i++) {
        for (j = 7; j >= 0; j--) {
            int bit = read_bit(dev);
            if (bit < 0) return -EIO;
            data[i] |= (bit << j);
        }
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    *humidity = (int32_t)raw_hum * 100 + (int32_t)raw_hum;
    *temperature = (int32_t)raw_temp * 100 + (int32_t)raw_temp;

    return 0;
}
