#include "pca9685.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>
#include <stddef.h>

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01

int pca9685_init(struct pca9685_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

static int pca9685_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int pca9685_write(struct pca9685_dev *dev, uint8_t *data, uint16_t len) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, addr, data, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int pca9685_read(struct pca9685_dev *dev, uint8_t *buf, uint16_t len) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(hi2c, addr, buf, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int pca9685_read_pwm(struct pca9685_dev *dev, uint8_t channel, uint16_t *out) {
    if (!dev || !out) return -1;
    if (channel > 1) return -1;
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t write_buf[1] = {reg};
    if (pca9685_write(dev, write_buf, 1) != 0) return -1;
    uint8_t read_buf[4];
    if (pca9685_read(dev, read_buf, 4) != 0) return -1;
    uint8_t off_l = read_buf[0];
    uint8_t off_h = read_buf[1];
    *out = (uint16_t)(((off_h & 0x0F) * 256) + off_l);
    return 0;
}