#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint16_t addr;
} pca9685_t;

int pca9685_init(pca9685_t *dev, i2c_t bus, uint16_t addr);
int pca9685_read_pwm_channel(pca9685_t *dev, uint8_t channel, uint16_t *value);

#endif /* PCA9685_H */
