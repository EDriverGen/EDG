#include "pca9685.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "threadx.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08

static int pca9685_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, dev_addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int pca9685_write(struct pca9685_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    uint8_t buffer[256];
    buffer[0] = reg;
    for (uint16_t i = 0; i < len; i++) {
        buffer[1 + i] = data[i];
    }
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, dev_addr, buffer, 1 + len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

int pca9685_init(struct pca9685_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *value)
{
    if (channel > 1) {
        return -1;
    }
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    int ret = pca9685_write_then_read(dev, reg, buf, 4);
    if (ret != 0) {
        return ret;
    }
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    uint16_t duty = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    *value = duty;
    return 0;
}
