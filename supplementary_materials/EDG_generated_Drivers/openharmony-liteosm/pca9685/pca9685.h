#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

struct pca9685_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int pca9685_init(struct pca9685_dev *dev, void *bus_handle);
int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out);

#endif