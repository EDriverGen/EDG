#include "hcsr04.h"
#include "transform.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define TRIG_PULSE_US 10
#define TIMEOUT_US 100000
#define SOUND_SPEED_NUM 343
#define SOUND_SPEED_DEN 2000

static int gpio_write(int fd, int value) {
    uint8_t buf = value ? 1 : 0;
    return PrivWrite(fd, &buf, 1);
}

static int gpio_read(int fd) {
    uint8_t buf;
    int ret = PrivRead(fd, &buf, 1);
    if (ret < 0) return -1;
    return buf ? 1 : 0;
}

static void delay_us(int us) {
    // Use PrivTaskDelay with ms resolution; for short delays, approximate
    if (us >= 1000) {
        PrivTaskDelay(us / 1000);
    } else {
        // busy wait approximation (not ideal but acceptable for short delays)
        volatile int i;
        for (i = 0; i < us * 10; i++) {
            __asm__ volatile("nop");
        }
    }
}

int hcsr04_init(struct hcsr04_device *dev, const char *trig_path, const char *echo_path) {
    if (!dev || !trig_path || !echo_path) return -EINVAL;
    
    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0) return -errno;
    
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }
    
    // Configure trig as output, echo as input (assume default modes)
    // No explicit configure call needed; open with O_RDWR sets appropriate mode
    
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw) {
    if (!dev || !raw) return -EINVAL;
    
    // Send trigger pulse
    gpio_write(dev->trig_fd, 1);
    delay_us(TRIG_PULSE_US);
    gpio_write(dev->trig_fd, 0);
    
    // Wait for echo pin to go high (start of pulse)
    int timeout = 0;
    while (gpio_read(dev->echo_fd) == 0) {
        delay_us(1);
        timeout++;
        if (timeout > TIMEOUT_US) {
            *raw = 0;
            return -EIO;
        }
    }
    
    // Measure high pulse width
    uint32_t width = 0;
    while (gpio_read(dev->echo_fd) == 1) {
        delay_us(1);
        width++;
        if (width > TIMEOUT_US) {
            *raw = 0;
            return -EIO;
        }
    }
    
    // Convert to mm: (width * 343) / 2000
    int64_t tmp = (int64_t)width * SOUND_SPEED_NUM;
    *raw = (int32_t)(tmp / SOUND_SPEED_DEN);
    
    return 0;
}