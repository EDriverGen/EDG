/*
 * DHT22 sensor driver for XiUOS
 */
#include "dht22_ref.h"

static int dht22_pin_read(int fd)
{ uint8_t val; PrivRead(fd, &val, 1); return val; }

static void dht22_pin_write(int fd, uint8_t val)
{ PrivWrite(fd, &val, 1); }

static void dht22_set_output(int fd)
{
    struct PinParam param;
    param.cmd = GPIO_CONFIG_MODE;
    param.mode = GPIO_CFG_OUTPUT;
    PrivIoctl(fd, 0, &param);
}

static void dht22_set_input(int fd)
{
    struct PinParam param;
    param.cmd = GPIO_CONFIG_MODE;
    param.mode = GPIO_CFG_INPUT_PULLUP;
    PrivIoctl(fd, 0, &param);
}

static int dht22_wait(int fd, int level, int max_us)
{
    int us = 0;
    while (dht22_pin_read(fd) == level) {
        dht22_delay_us(1); if (++us > max_us) return -1;
    }
    return us;
}

int dht22_init(struct dht22_device *dev, const char *gpio_path)
{
    if (!dev || !gpio_path) return -1;
    dev->gpio_fd = PrivOpen(gpio_path, O_RDWR);
    if (dev->gpio_fd < 0) return -1;
    dht22_pin_write(dev->gpio_fd, 1);
    return 0;
}

void dht22_deinit(struct dht22_device *dev)
{ if (dev && dev->gpio_fd >= 0) { PrivClose(dev->gpio_fd); dev->gpio_fd = -1; } }

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (!dev || dev->gpio_fd < 0 || !temp_x10 || !humidity_x10) return -1;

    dht22_set_output(dev->gpio_fd);
    dht22_pin_write(dev->gpio_fd, 0);
    PrivTaskDelay(2);
    dht22_pin_write(dev->gpio_fd, 1);
    dht22_delay_us(30);
    dht22_set_input(dev->gpio_fd);

    if (dht22_wait(dev->gpio_fd, 0, DHT22_TIMEOUT_US) < 0) return -1;
    if (dht22_wait(dev->gpio_fd, 1, DHT22_TIMEOUT_US) < 0) return -1;

    for (int i = 0; i < 40; i++) {
        if (dht22_wait(dev->gpio_fd, 0, DHT22_TIMEOUT_US) < 0) return -1;
        int high = dht22_wait(dev->gpio_fd, 1, DHT22_TIMEOUT_US);
        if (high < 0) return -1;
        data[i/8] <<= 1;
        if (high > DHT22_BIT_THRESHOLD) data[i/8] |= 1;
    }

    /* verify checksum */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return -1;
    /* humidity: data[0..1], 0.1% RH */
    *humidity_x10 = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    /* temperature: data[2..3], 0.1 degC, bit15=sign */
    uint16_t raw_t = (uint16_t)((uint16_t)data[2] << 8 | data[3]);
    if (raw_t & 0x8000)
        *temp_x10 = -(int16_t)(raw_t & 0x7FFF);
    else
        *temp_x10 = (int16_t)raw_t;
    return 0;
}
