#include "pca9685.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_READ_ADDR 0x71
#define MODE1_REG 0x00
#define MODE2_REG 0x01
#define LED0_OFF_L_REG 0x08

int pca9685_init(struct pca9685_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out) {
    if (!dev || !out) return -1;
    uint8_t reg = LED0_OFF_L_REG + (channel * 4);
    uint8_t buf[4];
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read(dev->bus_handle, PCA9685_READ_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, 4, 100);
    if (ret != HAL_OK) return -1;
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *out = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}
