#include "hcsr04.h"
#include <nuttx/ioexpander/gpio.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include "arch.h"

int hcsr04_init(struct hcsr04_dev *dev, const char *trig_path, const char *echo_path)
{
    int ret;

    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0) {
        return -errno;
    }

    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }

    ret = ioctl(dev->trig_fd, GPIOC_SETPINTYPE, 1);
    if (ret < 0) {
        close(dev->trig_fd);
        close(dev->echo_fd);
        return -errno;
    }

    ret = ioctl(dev->echo_fd, GPIOC_SETPINTYPE, 0);
    if (ret < 0) {
        close(dev->trig_fd);
        close(dev->echo_fd);
        return -errno;
    }

    return 0;
}

int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw)
{
    int ret;
    bool level;
    int timeout;
    uint32_t pulse_width_us = 0;

    ret = ioctl(dev->trig_fd, GPIOC_WRITE, 0);
    if (ret < 0) {
        return -errno;
    }

    ret = ioctl(dev->trig_fd, GPIOC_WRITE, 1);
    if (ret < 0) {
        return -errno;
    }

    for (volatile int i = 0; i < 10; i++);

    ret = ioctl(dev->trig_fd, GPIOC_WRITE, 0);
    if (ret < 0) {
        return -errno;
    }

    timeout = 10000;
    while (timeout-- > 0) {
        ret = ioctl(dev->echo_fd, GPIOC_READ, &level);
        if (ret < 0) {
            return -errno;
        }
        if (level) {
            break;
        }
        for (volatile int i = 0; i < 100; i++);
    }
    if (timeout <= 0) {
        return -EIO;
    }

    uint32_t count = 0;
    timeout = 100000;
    while (timeout-- > 0) {
        ret = ioctl(dev->echo_fd, GPIOC_READ, &level);
        if (ret < 0) {
            return -errno;
        }
        if (!level) {
            break;
        }
        count++;
        for (volatile int i = 0; i < 10; i++);
    }
    if (timeout <= 0) {
        return -EIO;
    }

    pulse_width_us = count;
    *raw = (int32_t)((pulse_width_us * 343) / 2000);
    return 0;
}