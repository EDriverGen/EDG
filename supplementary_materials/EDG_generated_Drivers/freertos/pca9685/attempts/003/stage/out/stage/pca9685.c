#include "pca9685.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_READ_ADDR 0x71
#define MODE1_REG 0x00
#define MODE2_REG 0x01
#define LED0_OFF_L_REG 0x08

static int i2c_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(dev->bus_handle, PCA9685_READ_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int i2c_write(struct pca9685_dev *dev, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, PCA9685_READ_ADDR << 1, data, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int i2c_read(struct pca9685_dev *dev, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(dev->bus_handle, PCA9685_READ_ADDR << 1, data, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

int pca9685_init(struct pca9685_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *value)
{
    uint8_t reg = LED0_OFF_L_REG + (channel * 4);
    uint8_t buf[4];
    int ret;

    // Write register pointer
    uint8_t cmd = reg;
    ret = i2c_write(dev, &cmd, 1);
    if (ret != 0) return -1;

    // Read 4 bytes (2 channels)
    ret = i2c_read(dev, buf, 4);
    if (ret != 0) return -1;

    // Decode little-endian: buf[0]=off_l, buf[1]=off_h, buf[2]=next_on_l, buf[3]=next_on_h
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1] & 0x0F;
    *value = (uint16_t)((off_h * 256) + off_l);

    return 0;
}
