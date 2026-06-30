/*
 * DHT22 sensor driver for NuttX (GPIO)
 */
#include "dht22_ref.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/ioexpander/gpio.h>

static int dht22_gpio_write(int fd, int val)
{
    bool v = val ? true : false;
    return ioctl(fd, GPIOC_WRITE, (unsigned long)&v);
}

static int dht22_gpio_read(int fd)
{
    bool v; ioctl(fd, GPIOC_READ, (unsigned long)&v); return v ? 1 : 0;
}

static void dht22_set_output(int fd)
{
    enum gpio_pintype_e pt = GPIO_OUTPUT_PIN;
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)pt);
}

static void dht22_set_input(int fd)
{
    enum gpio_pintype_e pt = GPIO_INPUT_PIN_PULLUP;
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)pt);
}

static int dht22_wait(int fd, int level, int max_us)
{
    int us = 0;
    while (dht22_gpio_read(fd) == level) {
        usleep(1); if (++us > max_us) return -1;
    }
    return us;
}

int dht22_init(struct dht22_device *dev, const char *gpio_path)
{
    if (!dev || !gpio_path) return -1;
    dev->data_fd = open(gpio_path, O_RDWR);
    if (dev->data_fd < 0) return -1;
    dht22_gpio_write(dev->data_fd, 1);
    return 0;
}

void dht22_deinit(struct dht22_device *dev)
{ if (dev && dev->data_fd >= 0) { close(dev->data_fd); dev->data_fd = -1; } }

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (!dev || dev->data_fd < 0 || !temp_x10 || !humidity_x10) return -1;

    /* start signal */
    dht22_set_output(dev->data_fd);
    dht22_gpio_write(dev->data_fd, 0);
    usleep(2000);
    dht22_gpio_write(dev->data_fd, 1);
    usleep(30);
    dht22_set_input(dev->data_fd);

    /* sensor response */
    if (dht22_wait(dev->data_fd, 0, DHT22_TIMEOUT_US) < 0) return -1;
    if (dht22_wait(dev->data_fd, 1, DHT22_TIMEOUT_US) < 0) return -1;

    /* read 40 bits */
    for (int i = 0; i < 40; i++) {
        if (dht22_wait(dev->data_fd, 0, DHT22_TIMEOUT_US) < 0) return -1;
        int high = dht22_wait(dev->data_fd, 1, DHT22_TIMEOUT_US);
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
