#include "pca9685.h"
#include <stdint.h>
#include <stddef.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"

#define PCA9685_ALL_LED_OFF_L 0xFC

int pca9685_init(struct pca9685_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

static int pca9685_read_regs(struct pca9685_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (HAL_I2C_Mem_Read(dev->bus_handle, (dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

int pca9685_read_pwm(struct pca9685_dev *dev, uint8_t channel, uint16_t *out) {
    if (!dev || !out || channel > 1) return -1;
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    if (pca9685_read_regs(dev, reg, buf, 4) != 0) return -1;
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *out = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}