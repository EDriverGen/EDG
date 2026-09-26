#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <drivers/dev_i2c.h>

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_ALL_LED_OFF_L 0xFC

struct rt_i2c_bus_device;

struct pca9685_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int pca9685_init(struct pca9685_device *dev, struct rt_i2c_bus_device *bus);
int pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel, uint16_t *pwm);

#endif