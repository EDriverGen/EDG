#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <stddef.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01

struct pca9685_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int pca9685_init(struct pca9685_dev *dev, void *bus_handle);
int pca9685_read_pwm(struct pca9685_dev *dev, uint8_t channel, uint16_t *out);

#endif