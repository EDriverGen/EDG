#include "hcsr04.h"
#include "transform.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define TRIG_PULSE_US 10
#define TIMEOUT_US 100000
#define SOUND_SPEED 343

static int gpio_write(int fd, int value) {
    uint8_t buf = value ? 1 : 0;
    return PrivWrite(fd, &buf, 1);
}

static int gpio_read(int fd) {
    uint8_t buf;
    if (PrivRead(fd, &buf, 1) != 1) return -1;
    return buf;
}

static int delay_us(int us) {
    struct timespec ts = {0, us * 1000};
    return nanosleep(&ts, NULL);
}

int hcsr04_init(struct hcsr04_device *dev, const char *trig_path, const char *echo_path) {
    if (!dev || !trig_path || !echo_path) return -EINVAL;
    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0) return -errno;
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) { close(dev->trig_fd); return -errno; }
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw) {
    if (!dev || !raw) return -EINVAL;
    int trig_fd = dev->trig_fd;
    int echo_fd = dev->echo_fd;

    // Ensure echo pin is input (already configured by system)
    // Send trigger pulse
    if (gpio_write(trig_fd, 1) < 0) return -EIO;
    delay_us(TRIG_PULSE_US);
    if (gpio_write(trig_fd, 0) < 0) return -EIO;

    // Wait for echo pin to go high
    int timeout = 0;
    while (gpio_read(echo_fd) == 0) {
        delay_us(1);
        if (++timeout > TIMEOUT_US) return -EIO;
    }

    // Measure high pulse width
    uint32_t pulse_width = 0;
    while (gpio_read(echo_fd) == 1) {
        delay_us(1);
        pulse_width++;
        if (pulse_width > TIMEOUT_US) return -EIO;
    }

    // Convert to mm: (pulse_width * 343) / 2000
    *raw = (int32_t)((pulse_width * SOUND_SPEED) / 2000);
    return 0;
}