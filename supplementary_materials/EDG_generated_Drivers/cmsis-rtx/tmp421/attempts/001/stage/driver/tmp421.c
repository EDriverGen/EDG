#include "tmp421.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#include "cmsis_rtx.h"
#define TMP421_I2C_ADDR 0x2A
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW 0x10
#define TMP421_REG_REMOTE_HIGH 0x01
#define TMP421_REG_REMOTE_LOW 0x11
#define TMP421_REG_STATUS 0x08
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_MANUFACTURER_ID_EXPECTED 0x55
#define TMP421_DEVICE_ID_EXPECTED 0x21
#define TMP421_CONVERSION_DELAY_MS 130

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_write(struct tmp421_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 2, 100);
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
    int timeout = 10;
    while (timeout--) {
        if (tmp421_read_reg(dev, TMP421_REG_STATUS, &status) != 0) {
            return -1;
        }
        if (!(status & 0x80)) {
            return 0;
        }
        HAL_Delay(15);
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
    uint8_t high_byte, low_byte;
    int16_t high_signed;
    int32_t combined;

    if (tmp421_poll_busy(dev) != 0) {
        return -1;
    }

    if (tmp421_read_reg(dev, high_reg, &high_byte) != 0) {
        return -1;
    }
    if (tmp421_read_reg(dev, low_reg, &low_byte) != 0) {
        return -1;
    }

    high_signed = (int16_t)(int8_t)high_byte;
    combined = ((int32_t)high_signed << 4) | ((low_byte >> 4) & 0x0F);
    *temp_milli = combined * 625 / 10;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    uint8_t val;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    if (tmp421_read_reg(dev, TMP421_REG_MANUFACTURER_ID, &val) != 0) {
        return -1;
    }
    if (val != TMP421_MANUFACTURER_ID_EXPECTED) {
        return -1;
    }

    if (tmp421_read_reg(dev, TMP421_REG_DEVICE_ID, &val) != 0) {
        return -1;
    }
    if (val != TMP421_DEVICE_ID_EXPECTED) {
        return -1;
    }

    return 0;
}

int tmp421_read_local(struct tmp421_dev *dev, int32_t *temp_local_val)
{
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_dev *dev, int32_t *temp_remote_val)
{
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE_HIGH, TMP421_REG_REMOTE_LOW, temp_remote_val);
}
