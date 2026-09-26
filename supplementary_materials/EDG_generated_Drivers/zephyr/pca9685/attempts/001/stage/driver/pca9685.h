#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_ALL_CALL_ADDR 0x70

#include <zephyr/device.h>
struct device;

struct pca9685_dev {
    const struct device *bus;
    uint8_t i2c_addr;
};

int pca9685_init(struct pca9685_dev *dev);
int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out);

#endif /* PCA9685_H */
