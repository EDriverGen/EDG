#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <stddef.h>

#include <hal/hal_i2c.h>
struct pca9685_dev {
    uint8_t i2c_num;
    uint8_t i2c_addr;
};

int pca9685_init(struct pca9685_dev *dev, void *bus_handle);
int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out);

#endif
