#include "dht22.h"
#include <nuttx/ioexpander/gpio.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include "arch.h"

#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int gpio_write(int fd, bool value)
{
    return ioctl(fd, GPIOC_WRITE, (unsigned long)value);
}

static int gpio_read(int fd, bool *value)
{
    return ioctl(fd, GPIOC_READ, (unsigned long)value);
}

static int gpio_set_dir(int fd, bool output)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)output);
}

static int wait_for_level(int fd, bool target, int timeout_us)
{
    bool level;
    int elapsed = 0;
    while (elapsed < timeout_us) {
        if (gpio_read(fd, &level) < 0)
            return -EIO;
        if (level == target)
            return 0;
        up_mdelay(0);
        elapsed += 1;
    }
    return -ETIMEDOUT;
}

static int measure_pulse_high(int fd, int timeout_us)
{
    bool level;
    int width = 0;
    while (width < timeout_us) {
        if (gpio_read(fd, &level) < 0)
            return -EIO;
        if (!level)
            break;
        width++;
    }
    return width;
}

int dht22_init(struct dht22_dev *dev, const char *trig_path, const char *echo_path)
{
    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0)
        return -errno;
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }
    /* Configure echo pin as input initially */
    if (gpio_set_dir(dev->echo_fd, false) < 0) {
        close(dev->trig_fd);
        close(dev->echo_fd);
        return -EIO;
    }
    /* Power-up delay */
    up_mdelay(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity_val, int32_t *temp_val)
{
    int ret;
    bool level;
    uint8_t data[5] = {0};
    int bit_idx;
    int byte_idx;
    int pulse_width;
    
    /* Send start signal: pull low for at least 18ms */
    if (gpio_set_dir(dev->echo_fd, true) < 0)
        return -EIO;
    if (gpio_write(dev->echo_fd, false) < 0)
        return -EIO;
    up_mdelay(DHT22_START_SIGNAL_LOW_MS);
    
    /* Release line and switch to input */
    if (gpio_write(dev->echo_fd, true) < 0)
        return -EIO;
    if (gpio_set_dir(dev->echo_fd, false) < 0)
        return -EIO;
    
    /* Wait for sensor response: pull low */
    ret = wait_for_level(dev->echo_fd, false, DHT22_RESPONSE_WAIT_US + DHT22_RESPONSE_LOW_US + DHT22_RESPONSE_HIGH_US);
    if (ret < 0)
        return -EIO;
    
    /* Wait for response low duration */
    ret = wait_for_level(dev->echo_fd, true, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US);
    if (ret < 0)
        return -EIO;
    
    /* Wait for response high duration */
    ret = wait_for_level(dev->echo_fd, false, DHT22_RESPONSE_HIGH_US + DHT22_TIMEOUT_US);
    if (ret < 0)
        return -EIO;
    
    /* Read 40 bits */
    for (bit_idx = 0; bit_idx < 40; bit_idx++) {
        /* Wait for low (start of bit) */
        ret = wait_for_level(dev->echo_fd, true, DHT22_BIT_LOW_US + DHT22_TIMEOUT_US);
        if (ret < 0)
            return -EIO;
        /* Wait for high */
        ret = wait_for_level(dev->echo_fd, false, DHT22_BIT_1_HIGH_US + DHT22_TIMEOUT_US);
        if (ret < 0)
            return -EIO;
        /* Measure high pulse width */
        pulse_width = measure_pulse_high(dev->echo_fd, DHT22_BIT_1_HIGH_US + DHT22_TIMEOUT_US);
        if (pulse_width < 0)
            return -EIO;
        /* Classify bit: if width > 50us (midpoint between 27 and 70) then 1 else 0 */
        if (pulse_width > 50) {
            data[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        }
        /* Wait for next low (end of bit) */
        ret = wait_for_level(dev->echo_fd, true, DHT22_BIT_LOW_US + DHT22_TIMEOUT_US);
        if (ret < 0)
            return -EIO;
    }
    
    /* Verify checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4])
        return -EIO;
    
    /* Convert to milli units */
    int32_t integral_rh = data[0];
    int32_t decimal_rh = data[1];
    int32_t integral_t = data[2];
    int32_t decimal_t = data[3];
    
    *humidity_val = integral_rh * 100 + decimal_rh;
    *temp_val = integral_t * 100 + decimal_t;
    
    return 0;
}