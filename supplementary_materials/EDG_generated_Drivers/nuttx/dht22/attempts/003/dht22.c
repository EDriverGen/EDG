#include "dht22.h"
#include "arch.h"
#include <errno.h>
#include <nuttx/ioexpander/gpio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define DHT22_TIMEOUT_US 200

static int gpio_write(int fd, bool value)
{
    return ioctl(fd, GPIOC_WRITE, (unsigned long)value);
}

static int gpio_read(int fd, bool *value)
{
    return ioctl(fd, GPIOC_READ, (unsigned long)value);
}

static int gpio_set_input(int fd)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)0);
}

static int gpio_set_output(int fd)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)1);
}

static int wait_for_level(int fd, bool target, unsigned int timeout_us)
{
    bool level;
    unsigned int elapsed = 0;
    while (elapsed < timeout_us) {
        int ret = gpio_read(fd, &level);
        if (ret < 0) return ret;
        if (level == target) return 0;
        up_mdelay(0);
        elapsed += 1;
    }
    return -ETIMEDOUT;
}

static int measure_pulse_width(int fd, bool level, unsigned int timeout_us, unsigned int *width_us)
{
    bool current;
    unsigned int elapsed = 0;
    while (elapsed < timeout_us) {
        int ret = gpio_read(fd, &current);
        if (ret < 0) return ret;
        if (current == level) break;
        up_mdelay(0);
        elapsed += 1;
    }
    if (elapsed >= timeout_us) return -ETIMEDOUT;
    unsigned int start = elapsed;
    while (elapsed < timeout_us) {
        int ret = gpio_read(fd, &current);
        if (ret < 0) return ret;
        if (current != level) break;
        up_mdelay(0);
        elapsed += 1;
    }
    if (elapsed >= timeout_us) return -ETIMEDOUT;
    *width_us = elapsed - start;
    return 0;
}

int dht22_init(struct dht22_dev *dev, const char *trig_path, const char *echo_path)
{
    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0) return -errno;
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }
    /* Configure echo pin as output initially for start signal */
    int ret = gpio_set_output(dev->echo_fd);
    if (ret < 0) {
        close(dev->trig_fd);
        close(dev->echo_fd);
        return ret;
    }
    /* Power-up delay */
    up_mdelay(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity_val, int32_t *temp_val)
{
    if (!dev || !humidity_val || !temp_val) return -EINVAL;
    int fd = dev->echo_fd;
    int ret;
    bool level;
    unsigned int width;
    uint8_t data[5] = {0};

    /* Send start signal: pull low for at least 18ms */
    ret = gpio_set_output(fd);
    if (ret < 0) return ret;
    ret = gpio_write(fd, false);
    if (ret < 0) return ret;
    up_mdelay(20);
    ret = gpio_write(fd, true);
    if (ret < 0) return ret;
    up_mdelay(0);

    /* Switch to input */
    ret = gpio_set_input(fd);
    if (ret < 0) return ret;

    /* Wait for sensor response: low for 80us */
    ret = wait_for_level(fd, false, DHT22_TIMEOUT_US);
    if (ret < 0) return ret;
    ret = measure_pulse_width(fd, false, 200, &width);
    if (ret < 0) return ret;
    if (width < 70 || width > 90) return -EIO;

    /* Wait for high for 80us */
    ret = wait_for_level(fd, true, DHT22_TIMEOUT_US);
    if (ret < 0) return ret;
    ret = measure_pulse_width(fd, true, 200, &width);
    if (ret < 0) return ret;
    if (width < 70 || width > 90) return -EIO;

    /* Read 40 bits */
    for (int i = 0; i < 40; i++) {
        /* Wait for low (50us) */
        ret = wait_for_level(fd, false, 100);
        if (ret < 0) return ret;
        /* Wait for high and measure */
        ret = wait_for_level(fd, true, 100);
        if (ret < 0) return ret;
        ret = measure_pulse_width(fd, true, 100, &width);
        if (ret < 0) return ret;
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        if (width >= 60) {
            data[byte_idx] |= (1 << bit_idx);
        }
    }

    /* Verify checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    /* Convert */
    uint16_t integral_rh = ((uint16_t)data[0] << 8) | data[1];
    uint16_t integral_t = ((uint16_t)data[2] << 8) | data[3];
    *humidity_val = (int32_t)integral_rh * 100 + (int32_t)data[1];
    *temp_val = (int32_t)integral_t * 100 + (int32_t)data[3];

    return 0;
}