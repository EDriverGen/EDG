#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

#include <drivers/dev_i2c.h>
struct rt_i2c_bus_device;

struct pca9685_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int pca9685_init(struct pca9685_device *dev, struct rt_i2c_bus_device *bus);
int pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel, uint16_t *pwm);

#endif
