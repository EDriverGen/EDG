#include "tmp421.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>
#include <stddef.h>

#define TMP421_I2C_ADDR 0x2A
#define TMP421_REG_LOCAL_TEMP_HIGH 0x00
#define TMP421_REG_LOCAL_TEMP_LOW  0x10
#define TMP421_REG_REMOTE_TEMP_HIGH 0x01
#define TMP421_REG_REMOTE_TEMP_LOW  0x11
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_REG_STATUS 0x08
#define TMP421_MANUFACTURER_ID_EXPECTED 0x55
#define TMP421_DEVICE_ID_EXPECTED 0x21
#define TMP421_CONVERSION_TIMEOUT_MS 130

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_write(struct tmp421_dev *dev, uint8_t reg, uint8_t data)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    uint8_t buf[2] = {reg, data};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, addr, buf, 2, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_read_reg(struct tmp421_dev *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int ret;
    uint32_t timeout = TMP421_CONVERSION_TIMEOUT_MS;
    while (timeout > 0) {
        ret = tmp421_read_reg(dev, TMP421_REG_STATUS, &status);
        if (ret != 0) return ret;
        if (!(status & 0x80)) return 0;
        HAL_Delay(1);
        timeout--;
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int ret;
    ret = tmp421_poll_busy(dev);
    if (ret != 0) return ret;
    ret = tmp421_write_then_read(dev, reg_high, &high_byte, 1);
    if (ret != 0) return ret;
    ret = tmp421_write_then_read(dev, reg_low, &low_byte, 1);
    if (ret != 0) return ret;
    int16_t raw = (int16_t)(((uint16_t)high_byte << 4) | ((uint16_t)(low_byte >> 4) & 0x0F));
    if (raw & 0x0800) {
        raw |= 0xF000;
    }
    int32_t milli = ((int32_t)raw * 625) / 10;
    *temp = milli;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;
    uint8_t val;
    int ret;
    ret = tmp421_read_reg(dev, TMP421_REG_MANUFACTURER_ID, &val);
    if (ret != 0 || val != TMP421_MANUFACTURER_ID_EXPECTED) return -1;
    ret = tmp421_read_reg(dev, TMP421_REG_DEVICE_ID, &val);
    if (ret != 0 || val != TMP421_DEVICE_ID_EXPECTED) return -1;
    return 0;
}

int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_TEMP_HIGH, TMP421_REG_LOCAL_TEMP_LOW, temp);
}

int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE_TEMP_HIGH, TMP421_REG_REMOTE_TEMP_LOW, temp);
}