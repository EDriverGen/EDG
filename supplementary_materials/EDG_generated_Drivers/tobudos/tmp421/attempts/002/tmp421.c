#include "tmp421.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>
#include <stddef.h>

#define TMP421_I2C_ADDR 0x2A
#define TMP421_ADDR_WRITE (TMP421_I2C_ADDR << 1)
#define TMP421_ADDR_READ ((TMP421_I2C_ADDR << 1) | 1)

#define REG_LOCAL_TEMP_HIGH 0x00
#define REG_REMOTE1_TEMP_HIGH 0x01
#define REG_STATUS 0x08
#define REG_LOCAL_TEMP_LOW 0x10
#define REG_REMOTE1_TEMP_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF

#define STATUS_BUSY 0x80

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(hi2c, TMP421_ADDR_WRITE, &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive(hi2c, TMP421_ADDR_READ, buf, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

static int tmp421_read_reg(struct tmp421_dev *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int timeout = 130;
    while (timeout--) {
        if (tmp421_read_reg(dev, REG_STATUS, &status) != 0) return -1;
        if (!(status & STATUS_BUSY)) return 0;
        HAL_Delay(1);
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int16_t raw;
    int32_t result;

    if (tmp421_poll_busy(dev) != 0) return -1;

    if (tmp421_write_then_read(dev, high_reg, &high_byte, 1) != 0) return -1;
    if (tmp421_write_then_read(dev, low_reg, &low_byte, 1) != 0) return -1;

    raw = (int16_t)((high_byte << 4) | (low_byte >> 4));
    if (raw & 0x0800) {
        raw |= 0xF000;
    }
    result = ((int32_t)raw * 625) / 10;
    *temp = result;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    uint8_t val;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    if (tmp421_read_reg(dev, REG_MANUFACTURER_ID, &val) != 0) return -1;
    if (val != 0x55) return -1;
    if (tmp421_read_reg(dev, REG_DEVICE_ID, &val) != 0) return -1;
    if (val != 0x21) return -1;

    return 0;
}

int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, REG_LOCAL_TEMP_HIGH, REG_LOCAL_TEMP_LOW, temp);
}

int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, REG_REMOTE1_TEMP_HIGH, REG_REMOTE1_TEMP_LOW, temp);
}