#include "tmp421.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_i2c.h"
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW  0x10
#define TMP421_REG_REMOTE1_HIGH 0x01
#define TMP421_REG_REMOTE1_LOW  0x11
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_REG_STATUS 0x08
#define TMP421_BUSY_BIT 0x80

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_read_byte(struct tmp421_dev *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int timeout = 130;
    while (timeout--) {
        if (tmp421_read_byte(dev, TMP421_REG_STATUS, &status) != 0) {
            return -1;
        }
        if (!(status & TMP421_BUSY_BIT)) {
            return 0;
        }
        HAL_Delay(1);
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int16_t raw;
    int32_t result;

    if (tmp421_poll_busy(dev) != 0) {
        return -1;
    }

    if (tmp421_write_then_read(dev, high_reg, &high_byte, 1) != 0) {
        return -1;
    }
    if (tmp421_write_then_read(dev, low_reg, &low_byte, 1) != 0) {
        return -1;
    }

    raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    result = ((int32_t)raw * 625) / 10;
    *temp = result;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    uint8_t val;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    if (tmp421_read_byte(dev, TMP421_REG_MANUFACTURER_ID, &val) != 0) {
        return -1;
    }
    if (val != 0x55) {
        return -1;
    }
    if (tmp421_read_byte(dev, TMP421_REG_DEVICE_ID, &val) != 0) {
        return -1;
    }
    if (val != 0x21) {
        return -1;
    }
    return 0;
}

int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp);
}

int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE1_HIGH, TMP421_REG_REMOTE1_LOW, temp);
}
