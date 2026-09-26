#include "dht22.h"
#include "transform.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define DHT22_TRIG_PATH "/dev/gpio_trig"
#define DHT22_ECHO_PATH "/dev/gpio_echo"

static int gpio_write(int fd, int value) {
    uint8_t buf = value ? 1 : 0;
    return PrivWrite(fd, &buf, 1);
}

static int gpio_read(int fd) {
    uint8_t buf;
    int ret = PrivRead(fd, &buf, 1);
    if (ret < 0) return ret;
    return buf ? 1 : 0;
}

int dht22_init(struct dht22_device *dev) {
    if (!dev) return -EINVAL;
    dev->trig_fd = open(DHT22_TRIG_PATH, O_RDWR);
    if (dev->trig_fd < 0) return -EIO;
    dev->echo_fd = open(DHT22_ECHO_PATH, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -EIO;
    }
    // Configure pins: set trig as output, echo as input
    // Using GPIO_CFG_INPUT = 0x01
    struct PinParam param;
    param.cmd = 0; // set mode
    param.mode = 0x01; // output for trig
    param.pin = 0;
    PrivWrite(dev->trig_fd, &param, sizeof(param));
    param.mode = 0x01; // input for echo
    PrivWrite(dev->echo_fd, &param, sizeof(param));
    // Power-up delay: 1 second
    PrivTaskDelay(1000);
    return 0;
}

static int wait_for_level(int fd, int level, int timeout_us) {
    int us = 0;
    while (us < timeout_us) {
        int val = gpio_read(fd);
        if (val < 0) return -EIO;
        if (val == level) return 0;
        usleep(1);
        us++;
    }
    return -ETIMEDOUT;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature) {
    if (!dev || !humidity || !temperature) return -EINVAL;
    uint8_t data[5] = {0};
    int bit_index;
    int byte_index = 0;
    int bit_count = 0;
    int val;

    // Send start signal: pull low for at least 18ms
    gpio_write(dev->trig_fd, 0);
    PrivTaskDelay(20); // 20ms > 18ms
    gpio_write(dev->trig_fd, 1);
    // Wait 30us for sensor response
    usleep(30);

    // Wait for sensor to pull low (response)
    if (wait_for_level(dev->echo_fd, 0, 200) != 0) {
        return -EIO;
    }
    // Wait for high (80us low then 80us high)
    if (wait_for_level(dev->echo_fd, 1, 200) != 0) {
        return -EIO;
    }
    // Wait for low (end of high)
    if (wait_for_level(dev->echo_fd, 0, 200) != 0) {
        return -EIO;
    }

    // Read 40 bits
    for (bit_index = 0; bit_index < 40; bit_index++) {
        // Wait for high (bit start low is 50us, then high)
        if (wait_for_level(dev->echo_fd, 1, 100) != 0) {
            return -EIO;
        }
        // Measure high pulse width
        int us = 0;
        while (us < 100) {
            val = gpio_read(dev->echo_fd);
            if (val < 0) return -EIO;
            if (val == 0) break;
            usleep(1);
            us++;
        }
        if (us >= 100) return -EIO;
        // Classify bit: if high > 40us then bit 1 else bit 0
        if (us > 40) {
            data[byte_index] |= (1 << (7 - bit_count));
        }
        bit_count++;
        if (bit_count == 8) {
            bit_count = 0;
            byte_index++;
        }
        // Wait for low (end of bit)
        if (wait_for_level(dev->echo_fd, 0, 100) != 0) {
            return -EIO;
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    // Convert to milli units
    int32_t integral_RH = data[0];
    int32_t decimal_RH = data[1];
    int32_t integral_T = data[2];
    int32_t decimal_T = data[3];

    *humidity = integral_RH * 100 + decimal_RH;
    *temperature = integral_T * 100 + decimal_T;

    return 0;
}