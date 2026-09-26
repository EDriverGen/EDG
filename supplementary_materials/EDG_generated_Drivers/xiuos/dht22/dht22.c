#include "dht22.h"
#include "transform.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int gpio_write(int fd, uint8_t val) {
    return PrivWrite(fd, &val, 1);
}

static int gpio_read(int fd, uint8_t *val) {
    return PrivRead(fd, val, 1);
}

static void gpio_set_mode(int fd, uint16_t mode) {
    struct PinParam param;
    param.cmd = GPIO_CONFIG_MODE;
    param.mode = mode;
    param.pin = 5;
    PrivIoctl(fd, 0, &param);
}

static int wait_for_level(int fd, uint8_t level, int timeout_us) {
    uint8_t val;
    int waited = 0;
    while (waited < timeout_us) {
        if (gpio_read(fd, &val) < 0) return -EIO;
        if (val == level) return waited;
        usleep(1);
        waited++;
    }
    return -ETIMEDOUT;
}

int dht22_init(struct dht22_device *dev) {
    if (!dev) return -EINVAL;
    dev->trig_fd = open("/dev/pin/PB5", 0);
    dev->echo_fd = open("/dev/pin/PB5", 0);
    if (dev->trig_fd < 0 || dev->echo_fd < 0) {
        if (dev->trig_fd >= 0) close(dev->trig_fd);
        if (dev->echo_fd >= 0) close(dev->echo_fd);
        return -ENODEV;
    }
    gpio_set_mode(dev->trig_fd, GPIO_CFG_OUTPUT);
    gpio_set_mode(dev->echo_fd, GPIO_CFG_INPUT_PULLUP);
    PrivTaskDelay(1000); // power-up delay
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (!dev || !humidity_val || !temp_val) return -EINVAL;
    uint8_t data[5] = {0};
    int ret;

    // Send start signal: pull low for at least 18ms
    uint8_t low = 0;
    gpio_set_mode(dev->trig_fd, GPIO_CFG_OUTPUT);
    if (gpio_write(dev->trig_fd, low) < 0) return -EIO;
    PrivTaskDelay(DHT22_START_SIGNAL_LOW_MS);
    // Release line (set high)
    uint8_t high = 1;
    if (gpio_write(dev->trig_fd, high) < 0) return -EIO;
    gpio_set_mode(dev->echo_fd, GPIO_CFG_INPUT_PULLUP);

    // Wait for sensor response: line should go low within 20-40us
    ret = wait_for_level(dev->echo_fd, 0, 50);
    if (ret < 0) return -EIO;
    // Wait for response low duration (80us)
    ret = wait_for_level(dev->echo_fd, 1, 150);
    if (ret < 0) return -EIO;
    // Wait for response high duration (80us)
    ret = wait_for_level(dev->echo_fd, 0, 150);
    if (ret < 0) return -EIO;

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        // Wait for bit start low (50us)
        ret = wait_for_level(dev->echo_fd, 1, 100);
        if (ret < 0) return -EIO;
        // Wait for high pulse and measure its duration
        uint8_t val;
        int high_start = 0;
        while (1) {
            if (gpio_read(dev->echo_fd, &val) < 0) return -EIO;
            if (val == 1) break;
            high_start++;
            if (high_start > 100) return -EIO;
            usleep(1);
        }
        int high_count = 0;
        while (1) {
            if (gpio_read(dev->echo_fd, &val) < 0) return -EIO;
            if (val == 0) break;
            high_count++;
            if (high_count > 100) return -EIO;
            usleep(1);
        }
        // Classify bit: if high pulse > 40us (threshold between 28 and 70), bit=1 else 0
        int bit = (high_count > 40) ? 1 : 0;
        int byte_idx = i / 8;
        data[byte_idx] = (data[byte_idx] << 1) | bit;
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    // Convert to milli units
    uint16_t integral_RH = ((uint16_t)data[0] << 8) | data[1];
    uint16_t integral_T = ((uint16_t)data[2] << 8) | data[3];
    int32_t temp_x10 = (integral_T & 0x8000)
        ? -(int32_t)(integral_T & 0x7FFF)
        : (int32_t)integral_T;

    *humidity_val = (int32_t)integral_RH * 100;
    *temp_val = temp_x10 * 100;

    return 0;
}
