#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

struct i2c_master_s;

struct pca9685_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int pca9685_init(struct pca9685_dev_s *dev, struct i2c_master_s *bus);
int pca9685_read_pwm_channel(struct pca9685_dev_s *dev, uint8_t channel, uint16_t *duty);

#endif /* PCA9685_H */