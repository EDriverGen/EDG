#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

struct pca9685_device {
    int fd;
    uint8_t addr;
};

int pca9685_init(struct pca9685_device *dev, void *bus_handle);
int pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *value);

#endif /* PCA9685_H */