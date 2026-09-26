#include "dht22.h"
#include <errno.h>
#include <stdint.h>

#include <hal/hal_gpio.h>
#include "apache_mynewt.h"
#include <os/os_time.h>
#define DHT22_DATA_PIN PB5

static int dht22_wait_for_level(int pin, int level, uint32_t timeout_us)
{
    uint32_t wait = 0;
    while (hal_gpio_read(pin) != level) {
        os_cputime_delay_usecs(1);
        wait++;
        if (wait > timeout_us) {
            return -EIO;
        }
    }
    return 0;
}

int dht22_init(struct dht22_dev *dev, int data_pin)
{
    dev->data_pin = data_pin;
    hal_gpio_init_in(dev->data_pin, HAL_GPIO_PULL_NONE);
    os_time_delay(OS_TICKS_PER_SEC);
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity, int32_t *temperature)
{
    int pin = dev->data_pin;
    uint8_t data[5] = {0};
    int ret;

    // Send start signal: pull low for at least 18ms
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE);
    hal_gpio_write(pin, 0);
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE);
    hal_gpio_write(pin, 0);
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 18);

    // Pull high and wait for sensor response
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE);
    hal_gpio_write(pin, 1);
    os_cputime_delay_usecs(30);

    // Wait for sensor to pull low (response)
    ret = dht22_wait_for_level(pin, 0, 200);
    if (ret) return ret;
    // Wait for sensor to pull high
    ret = dht22_wait_for_level(pin, 1, 200);
    if (ret) return ret;

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        // Wait for low
        ret = dht22_wait_for_level(pin, 0, 100);
        if (ret) return ret;
        // Wait for high
        ret = dht22_wait_for_level(pin, 1, 100);
        if (ret) return ret;
        // Measure high pulse width
        uint32_t width = 0;
        while (hal_gpio_read(pin) == 1) {
            os_cputime_delay_usecs(1);
            width++;
            if (width > 100) break;
        }
        // Determine bit: if width > 30us, it's 1
        int bit = (width > 30) ? 1 : 0;
        data[i / 8] = (data[i / 8] << 1) | bit;
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    // Convert to milli units
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    *humidity = (int32_t)raw_hum * 100 + (int32_t)raw_hum;
    *temperature = (int32_t)raw_temp * 100 + (int32_t)raw_temp;

    return 0;
}
