#include "dht22.h"
#include "transform.h"
#include <errno.h>
#include <stdint.h>

#define DHT22_START_SIGNAL_LOW_MS 1
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int gpio_write(int fd, uint8_t value)
{
    return PrivWrite(fd, &value, 1);
}

static int gpio_read(int fd, uint8_t *value)
{
    return PrivRead(fd, value, 1);
}

static int wait_for_level(int fd, int target, int timeout_us)
{
    uint8_t level;
    int elapsed = 0;
    while (elapsed < timeout_us) {
        if (gpio_read(fd, &level) < 0)
            return -EIO;
        if ((int)level == target)
            return 0;
        elapsed++;
    }
    return -ETIMEDOUT;
}

static int measure_pulse_high(int fd, int timeout_us)
{
    uint8_t level;
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

int dht22_init(struct dht22_device *dev)
{
    if (!dev) return -EINVAL;
    dev->trig_fd = open("/dev/PB5", 0);
    dev->echo_fd = open("/dev/PB5", 0);
    if (dev->trig_fd < 0 || dev->echo_fd < 0) {
        if (dev->trig_fd >= 0) close(dev->trig_fd);
        if (dev->echo_fd >= 0) close(dev->echo_fd);
        return -ENODEV;
    }
    PrivTaskDelay(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    if (!dev || !humidity_val || !temp_val) return -EINVAL;
    int ret, pulse_width;
    uint8_t data[5] = {0};

    /* Send start signal: pull low, then release. */
    uint8_t low = 0;
    if (gpio_write(dev->trig_fd, low) < 0) return -EIO;
    PrivTaskDelay(DHT22_START_SIGNAL_LOW_MS);
    uint8_t high = 1;
    if (gpio_write(dev->trig_fd, high) < 0) return -EIO;

    /* DHT22 response handshake: LOW 80us, HIGH 80us. */
    ret = wait_for_level(dev->echo_fd, 0, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US);
    if (ret < 0) return -EIO;
    ret = wait_for_level(dev->echo_fd, 1, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US);
    if (ret < 0) return -EIO;
    ret = wait_for_level(dev->echo_fd, 0, DHT22_RESPONSE_HIGH_US + DHT22_TIMEOUT_US);
    if (ret < 0) return -EIO;

    /* Read 40 bits: detect rising edge, measure HIGH pulse, classify. */
    for (int i = 0; i < 40; i++) {
        ret = wait_for_level(dev->echo_fd, 1, DHT22_BIT_1_HIGH_US + DHT22_TIMEOUT_US);
        if (ret < 0) return -EIO;
        pulse_width = measure_pulse_high(dev->echo_fd, DHT22_BIT_1_HIGH_US + DHT22_TIMEOUT_US);
        if (pulse_width < 0) return -EIO;
        if (pulse_width > 50) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    /* Checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -EIO;

    /* DHT22 format: MSB-first, value x10. Convert to milli units. */
    int32_t raw_RH = ((int32_t)data[0] << 8) | data[1];
    int32_t raw_T  = ((int32_t)data[2] << 8) | data[3];
    if (raw_T & 0x8000) raw_T = -(raw_T & 0x7FFF);
    *humidity_val = raw_RH * 100;
    *temp_val     = raw_T * 100;

    return 0;
}
