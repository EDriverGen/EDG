#include "pca9685.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "threadx.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_PRE_SCALE 0xFE

static int pca9685_write_reg(struct pca9685_dev *dev, uint8_t reg, uint8_t value)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t data[2] = {reg, value};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, (uint16_t)(dev->i2c_addr << 1), data, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int pca9685_read_reg(struct pca9685_dev *dev, uint8_t reg, uint8_t *value)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, value, 1, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int pca9685_init(struct pca9685_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *value)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    HAL_StatusTypeDef ret;

    // Write register pointer
    ret = HAL_I2C_Master_Transmit(hi2c, (uint16_t)(dev->i2c_addr << 1), &reg, 1, 100);
    if (ret != HAL_OK) return -1;

    // Read 4 bytes: off_l, off_h, next_on_l, next_on_h (auto-increment)
    ret = HAL_I2C_Master_Receive(hi2c, (uint16_t)(dev->i2c_addr << 1), buf, 4, 100);
    if (ret != HAL_OK) return -1;

    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1] & 0x0F;
    *value = (uint16_t)((off_h * 256) + off_l);
    return 0;
}
