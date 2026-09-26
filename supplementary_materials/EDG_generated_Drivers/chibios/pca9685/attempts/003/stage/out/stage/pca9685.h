#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <stddef.h>

struct pca9685_device {
    void *bus_handle;
    uint8_t i2c_addr;
};

void pca9685_init(struct pca9685_device *dev, void *bus_handle);
int pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *out);

#endif /* PCA9685_H */