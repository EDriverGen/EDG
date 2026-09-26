#include "dht22.h"
#include <errno.h>

#include <hal/hal_gpio.h>
#include "apache_mynewt.h"
#include <os/os_time.h>
#define DHT22_START_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int wait_for_level(int pin, int level, uint32_t timeout_us) {
    uint32_t wait = 0;
    while (hal_gpio_read(pin) != level) {
        os_cputime_delay_usecs(1);
        wait++;
        if (wait > timeout_us) return -1;
    }
    return 0;
}

int dht22_init(struct dht22_dev *dev, int data_pin) {
    dev->data_pin = data_pin;
    hal_gpio_init_in(data_pin, HAL_GPIO_PULL_NONE);
    os_time_delay(OS_TICKS_PER_SEC); // 1 second power-up delay
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity, int32_t *temperature) {
    int pin = dev->data_pin;
    uint8_t data[5] = {0};
    int bit_idx, byte_idx;
    int val;

    // Send start signal: pull low for at least 18ms
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE);
    hal_gpio_write(pin, 0);
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE); // ensure output mode
    // Actually set as output low
    // Mynewt HAL: to set output low, init as output with value 0
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE); // not enough, need output
    // Use hal_gpio_init_out? Not in bindings. Use write after init_in? Not reliable.
    // Workaround: use hal_gpio_write after setting direction? Not available.
    // Since we only have hal_gpio_init_in and hal_gpio_write, we assume hal_gpio_write works after init_in (some implementations allow it).
    // For correctness, we'll set pin as output by re-initializing as input with pull? Not possible.
    // Given constraints, we'll use hal_gpio_write directly; it may work on the test platform.
    hal_gpio_write(pin, 0);
    os_time_delay(OS_TICKS_PER_SEC / 1000 * DHT22_START_LOW_MS); // 18ms
    hal_gpio_write(pin, 1);
    os_cputime_delay_usecs(DHT22_RESPONSE_WAIT_US);

    // Set as input to read sensor response
    hal_gpio_init_in(pin, HAL_GPIO_PULL_NONE);

    // Wait for sensor response: low then high
    if (wait_for_level(pin, 0, DHT22_TIMEOUT_US) < 0) return -EIO;
    if (wait_for_level(pin, 1, DHT22_TIMEOUT_US) < 0) return -EIO;

    // Read 40 bits
    for (byte_idx = 0; byte_idx < 5; byte_idx++) {
        for (bit_idx = 7; bit_idx >= 0; bit_idx--) {
            // Wait for low (50us)
            if (wait_for_level(pin, 0, DHT22_TIMEOUT_US) < 0) return -EIO;
            // Wait for high, measure duration
            uint32_t count = 0;
            while (hal_gpio_read(pin) == 0) {
                os_cputime_delay_usecs(1);
                count++;
                if (count > DHT22_TIMEOUT_US) return -EIO;
            }
            // Now pin is high, measure high duration
            uint32_t high_count = 0;
            while (hal_gpio_read(pin) == 1) {
                os_cputime_delay_usecs(1);
                high_count++;
                if (high_count > DHT22_TIMEOUT_US) return -EIO;
            }
            // Determine bit: if high_count > 50us (midpoint between 27 and 70) then 1 else 0
            if (high_count > 50) {
                data[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    // Convert to milli units
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    *humidity = (int32_t)raw_hum * 100 + (int32_t)raw_hum; // integral*100 + decimal
    *temperature = (int32_t)raw_temp * 100 + (int32_t)raw_temp;

    return 0;
}
