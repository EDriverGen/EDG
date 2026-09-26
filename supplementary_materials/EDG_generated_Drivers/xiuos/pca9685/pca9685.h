#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <stddef.h>
#include "bus_i2c.h"

struct I2cBus;

struct pca9685_device {
    struct I2cBus *bus;
    uint8_t addr;
    int fd;
};

int pca9685_init(struct pca9685_device *dev, struct I2cBus *bus_handle);
int pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *value);

#endif