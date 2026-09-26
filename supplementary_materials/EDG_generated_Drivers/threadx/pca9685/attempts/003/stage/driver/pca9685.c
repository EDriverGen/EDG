#include "pca9685.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "threadx.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_MODE1 0x00
#define PCA9685_MODE1_SLEEP 0x10

static int pca9685_write_reg(struct pca9685_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int pca9685_read_regs(struct pca9685_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
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
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    int ret = pca9685_read_regs(dev, reg, buf, 4);
    if (ret != 0) return ret;
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *value = (uint16_t)(((off_h & 0x0F) * 256) + off_l);
    return 0;
}
