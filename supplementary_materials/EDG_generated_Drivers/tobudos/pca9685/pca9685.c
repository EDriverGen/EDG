#include "pca9685.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_READ_ADDR 0x71
#define PCA9685_LED0_OFF_L 0x08

int pca9685_init(struct pca9685_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm(struct pca9685_dev *dev, uint8_t channel, uint16_t *out) {
    if (!dev || !dev->bus_handle || !out) return -1;
    if (channel > 1) return -1;
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(hi2c, PCA9685_READ_ADDR, &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive(hi2c, PCA9685_READ_ADDR, buf, 4, 100);
    if (ret != HAL_OK) return -1;
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *out = (uint16_t)(((off_h & 0x0F) * 256) + off_l);
    return 0;
}