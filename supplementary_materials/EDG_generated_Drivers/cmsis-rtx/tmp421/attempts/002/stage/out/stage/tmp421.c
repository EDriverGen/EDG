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

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, addr, (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_write(struct tmp421_dev *dev, uint8_t reg, uint8_t value)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    uint8_t buf[2] = {reg, value};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, addr, buf, 2, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int timeout = 200;
    while (timeout--) {
        if (tmp421_write_then_read(dev, TMP421_REG_STATUS, &status, 1) != 0) {
            return -1;
        }
        if (!(status & 0x80)) {
            return 0;
        }
        HAL_Delay(1);
    }
    return -1;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    uint8_t id;
    if (tmp421_write_then_read(dev, TMP421_REG_MANUFACTURER_ID, &id, 1) != 0) {
        return -1;
    }
    if (id != TMP421_MANUFACTURER_ID_EXPECTED) {
        return -1;
    }
    if (tmp421_write_then_read(dev, TMP421_REG_DEVICE_ID, &id, 1) != 0) {
        return -1;
    }
    if (id != TMP421_DEVICE_ID_EXPECTED) {
        return -1;
    }

    return 0;
}

int tmp421_read_local(struct tmp421_dev *dev, int32_t *temp_local_val)
{
    if (tmp421_poll_busy(dev) != 0) {
        return -1;
    }

    uint8_t high_byte, low_byte;
    if (tmp421_write_then_read(dev, TMP421_REG_LOCAL_HIGH, &high_byte, 1) != 0) {
        return -1;
    }
    if (tmp421_write_then_read(dev, TMP421_REG_LOCAL_LOW, &low_byte, 1) != 0) {
        return -1;
    }

    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    *temp_local_val = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_read_remote(struct tmp421_dev *dev, int32_t *temp_remote_val)
{
    if (tmp421_poll_busy(dev) != 0) {
        return -1;
    }

    uint8_t high_byte, low_byte;
    if (tmp421_write_then_read(dev, TMP421_REG_REMOTE_HIGH, &high_byte, 1) != 0) {
        return -1;
    }
    if (tmp421_write_then_read(dev, TMP421_REG_REMOTE_LOW, &low_byte, 1) != 0) {
        return -1;
    }

    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    *temp_remote_val = ((int32_t)raw * 625) / 10;
    return 0;
}
