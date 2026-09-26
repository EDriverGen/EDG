#include "dht22.h"
#include "transform.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define DHT22_TRIG_PATH "/dev/gpio_trig"
#define DHT22_ECHO_PATH "/dev/gpio_echo"

static int gpio_write(int fd, uint8_t val) {
    return PrivWrite(fd, &val, 1);
}

static int gpio_read(int fd, uint8_t *val) {
    return PrivRead(fd, val, 1);
}

static int wait_for_level(int fd, uint8_t level, int timeout_us) {
    uint8_t val;
    int waited = 0;
    while (waited < timeout_us) {
        if (gpio_read(fd, &val) < 0) return -EIO;
        if (val == level) return 0;
        usleep(1);
        waited++;
    }
    return -ETIMEDOUT;
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
    // Power-up delay: wait 1 second
    PrivTaskDelay(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature) {
    if (!dev || !humidity || !temperature) return -EINVAL;
    int trig_fd = dev->trig_fd;
    int echo_fd = dev->echo_fd;
    uint8_t buf[5];
    int bit_index, byte_index;
    uint8_t val;
    int ret;

    // Send start signal: pull low for at least 18ms
    gpio_write(trig_fd, 0);
    PrivTaskDelay(20); // 20ms > 18ms
    // Release line (set high)
    gpio_write(trig_fd, 1);
    // Wait 30us for sensor response
    usleep(30);

    // Wait for sensor to pull low (response low)
    ret = wait_for_level(echo_fd, 0, 200);
    if (ret < 0) return -EIO;
    // Wait for sensor to release (response high)
    ret = wait_for_level(echo_fd, 1, 200);
    if (ret < 0) return -EIO;

    // Read 40 bits
    for (bit_index = 0; bit_index < 40; bit_index++) {
        // Wait for low (start of bit)
        ret = wait_for_level(echo_fd, 0, 100);
        if (ret < 0) return -EIO;
        // Wait for high
        ret = wait_for_level(echo_fd, 1, 100);
        if (ret < 0) return -EIO;
        // Measure high pulse width
        int high_time = 0;
        while (high_time < 100) {
            if (gpio_read(echo_fd, &val) < 0) return -EIO;
            if (val == 0) break;
            usleep(1);
            high_time++;
        }
        // Classify bit: if high_time > 40us (threshold between 28 and 70), bit=1 else 0
        int bit = (high_time > 40) ? 1 : 0;
        byte_index = bit_index / 8;
        if (bit) {
            buf[byte_index] |= (1 << (7 - (bit_index % 8)));
        } else {
            buf[byte_index] &= ~(1 << (7 - (bit_index % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = buf[0] + buf[1] + buf[2] + buf[3];
    if (sum != buf[4]) return -EIO;

    // Convert to milli units
    uint16_t integral_rh = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t integral_t = ((uint16_t)buf[2] << 8) | buf[3];
    *humidity = (int32_t)integral_rh * 100 + (int32_t)integral_rh;
    *temperature = (int32_t)integral_t * 100 + (int32_t)integral_t;

    return 0;
}