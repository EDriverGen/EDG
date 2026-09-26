#include "pca9685.h"
#include "stm32f1xx_hal.h"
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
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

static int pca9685_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *data, uint16_t len) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, PCA9685_READ_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

static int pca9685_write(struct pca9685_dev *dev, uint8_t reg, uint8_t *data, uint16_t len) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t buf[1 + len];
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, PCA9685_READ_ADDR, buf, 1 + len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *value) {
    if (!dev || !value) return -1;
    if (channel > 1) return -1;
    uint8_t reg = LED0_OFF_L_REG + (channel * 4);
    uint8_t buf[4];
    int ret = pca9685_write(dev, reg, NULL, 0);
    if (ret != 0) return -1;
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef hal_ret = HAL_I2C_Master_Receive(hi2c, PCA9685_READ_ADDR, buf, 4, 100);
    if (hal_ret != HAL_OK) return -1;
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *value = ((off_h & 0x0F) * 256) + off_l;
    return 0;
}
