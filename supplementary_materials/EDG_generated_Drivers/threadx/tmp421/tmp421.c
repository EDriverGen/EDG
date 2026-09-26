#include "tmp421.h"
#include <stddef.h>
#include "stm32f1xx_hal.h"

#include "threadx.h"
#define TMP421_I2C_ADDR 0x2A
#define TMP421_REG_STATUS 0x08
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW 0x10
#define TMP421_REG_REMOTE1_HIGH 0x01
#define TMP421_REG_REMOTE1_LOW 0x11
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_TIMEOUT_MS 130
#define TMP421_I2C_TIMEOUT 100

static int tmp421_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, buf, len, TMP421_I2C_TIMEOUT);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_write_only(struct tmp421_device *dev, uint8_t reg, uint8_t value)
{
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), data, 2, TMP421_I2C_TIMEOUT);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    uint32_t start = HAL_GetTick();
    do {
        if (tmp421_write_then_read(dev, TMP421_REG_STATUS, &status, 1) != 0) {
            return -1;
        }
        if (!(status & TMP421_BUSY_BIT)) {
            return 0;
        }
        HAL_Delay(1);
    } while ((HAL_GetTick() - start) < TMP421_CONVERSION_TIMEOUT_MS);
    return -1;
}

int tmp421_init(struct tmp421_device *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    uint8_t id;
    if (tmp421_write_then_read(dev, TMP421_REG_MANUFACTURER_ID, &id, 1) != 0) {
        return -1;
    }
    if (id != 0x55) {
        return -1;
    }
    if (tmp421_write_then_read(dev, TMP421_REG_DEVICE_ID, &id, 1) != 0) {
        return -1;
    }
    if (id != 0x21) {
        return -1;
    }

    return 0;
}

int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *temp_local_val)
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

int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *temp_remote_val)
{
    if (tmp421_poll_busy(dev) != 0) {
        return -1;
    }

    uint8_t high_byte, low_byte;
    if (tmp421_write_then_read(dev, TMP421_REG_REMOTE1_HIGH, &high_byte, 1) != 0) {
        return -1;
    }
    if (tmp421_write_then_read(dev, TMP421_REG_REMOTE1_LOW, &low_byte, 1) != 0) {
        return -1;
    }

    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    *temp_remote_val = ((int32_t)raw * 625) / 10;
    return 0;
}
