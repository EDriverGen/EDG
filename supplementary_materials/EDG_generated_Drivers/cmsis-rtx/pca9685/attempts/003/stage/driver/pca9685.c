#include "pca9685.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "cmsis_rtx.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08

static int pca9685_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *data, uint16_t len) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    if (HAL_I2C_Mem_Read(hi2c, (dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

static int pca9685_write(struct pca9685_dev *dev, uint8_t reg, uint8_t *data, uint16_t len) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data[0];
    if (HAL_I2C_Master_Transmit(hi2c, (dev->i2c_addr << 1), buf, len + 1, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

int pca9685_init(struct pca9685_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *value) {
    uint8_t buf[4];
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    
    // Write register pointer
    if (HAL_I2C_Master_Transmit(hi2c, (dev->i2c_addr << 1), &reg, 1, 100) != HAL_OK) {
        return -1;
    }
    // Read 4 bytes (OFF_L, OFF_H, next ON_L, next ON_H)
    if (HAL_I2C_Master_Receive(hi2c, (dev->i2c_addr << 1), buf, 4, 100) != HAL_OK) {
        return -1;
    }
    
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *value = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}
