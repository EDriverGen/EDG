#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <stdbool.h>
#include "bus_i2c.h"

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_ALLCALL_ADDR 0x70
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_PRE_SCALE 0xFE

struct I2cBus;

struct pca9685_device {
    struct I2cBus *bus;
    int fd;
    uint8_t i2c_addr;
};

int pca9685_init(struct pca9685_device *dev, struct I2cBus *bus_handle);
int pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *value);

#endif