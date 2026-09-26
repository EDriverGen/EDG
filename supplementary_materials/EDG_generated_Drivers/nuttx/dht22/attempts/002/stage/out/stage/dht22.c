#include "dht22.h"
#include "arch.h"
#include <errno.h>
#include <nuttx/ioexpander/gpio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define DHT22_POWERUP_DELAY_MS 1000
#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int gpio_set_output(int fd, bool value)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)0);
}

static int gpio_set_input(int fd)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)1);
}

static int gpio_write(int fd, bool value)
{
    return ioctl(fd, GPIOC_WRITE, (unsigned long)value);
}

static int gpio_read(int fd, bool *value)
{
    return ioctl(fd, GPIOC_READ, (unsigned long)value);
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
        up_mdelay(0);
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
    up_mdelay(DHT22_POWERUP_DELAY_MS);
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity_val, int32_t *temp_val)
{
    int ret;
    bool level;
    uint8_t data[5];
    int bit_idx;
    int byte_idx;
    int pulse_width;
    
    /* Send start signal: pull low for at least 18ms */
    ret = gpio_set_output(dev->trig_fd, 0);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    up_mdelay(DHT22_START_SIGNAL_LOW_MS);
    
    /* Release line and switch to input */
    ret = gpio_set_input(dev->echo_fd);
    if (ret < 0) return ret;
    
    /* Wait for sensor response: pull low */
    ret = wait_for_level(dev->echo_fd, false, DHT22_RESPONSE_WAIT_US + 100);
    if (ret < 0) return -EIO;
    
    /* Wait for response low duration */
    ret = wait_for_level(dev->echo_fd, true, DHT22_RESPONSE_LOW_US + 100);
    if (ret < 0) return -EIO;
    
    /* Wait for response high duration */
    ret = wait_for_level(dev->echo_fd, false, DHT22_RESPONSE_HIGH_US + 100);
    if (ret < 0) return -EIO;
    
    /* Read 40 bits */
    for (bit_idx = 0; bit_idx < 40; bit_idx++) {
        /* Wait for bit low */
        ret = wait_for_level(dev->echo_fd, true, DHT22_BIT_LOW_US + 100);
        if (ret < 0) return -EIO;
        /* Wait for bit high */
        ret = wait_for_level(dev->echo_fd, false, DHT22_BIT_1_HIGH_US + 100);
        if (ret < 0) return -EIO;
        /* Measure high pulse width */
        pulse_width = measure_pulse_high(dev->echo_fd, DHT22_BIT_1_HIGH_US + 50);
        if (pulse_width < 0) return pulse_width;
        
        byte_idx = bit_idx / 8;
        data[byte_idx] <<= 1;
        if (pulse_width > (DHT22_BIT_0_HIGH_US + DHT22_BIT_1_HIGH_US) / 2)
            data[byte_idx] |= 1;
    }
    
    /* Verify checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4])
        return -EIO;
    
    /* Convert humidity: integral_RH * 100 + decimal_RH */
    uint16_t integral_rh = (uint16_t)data[0];
    uint16_t decimal_rh = (uint16_t)data[1];
    *humidity_val = (int32_t)(integral_rh * 100 + decimal_rh);
    
    /* Convert temperature: integral_T * 100 + decimal_T */
    uint16_t integral_t = (uint16_t)data[2];
    uint16_t decimal_t = (uint16_t)data[3];
    *temp_val = (int32_t)(integral_t * 100 + decimal_t);
    
    return 0;
}