#include "tmp421.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#define TMP421_I2C_ADDR 0x2A
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW  0x10
#define TMP421_REG_REMOTE1_HIGH 0x01
#define TMP421_REG_REMOTE1_LOW  0x11
#define TMP421_REG_STATUS 0x08
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_DELAY_MS 130

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle,
                           (uint16_t)(dev->i2c_addr << 1),
                           (uint16_t)reg,
                           I2C_MEMADD_SIZE_8BIT,
                           buf,
                           len,
                           100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int tmp421_write(struct tmp421_dev *dev, uint8_t reg, uint8_t value)
{
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                  (uint16_t)(dev->i2c_addr << 1),
                                  data,
                                  2,
                                  100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int tmp421_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return tmp421_write_then_read(dev, reg, buf, len);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int ret;
    for (int i = 0; i < 10; i++) {
        ret = tmp421_read(dev, TMP421_REG_STATUS, &status, 1);
        if (ret != 0) return -1;
        if (!(status & TMP421_BUSY_BIT)) return 0;
        HAL_Delay(TMP421_CONVERSION_DELAY_MS);
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int ret;
    ret = tmp421_read(dev, high_reg, &high_byte, 1);
    if (ret != 0) return -1;
    ret = tmp421_read(dev, low_reg, &low_byte, 1);
    if (ret != 0) return -1;
    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    *temp = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;
    uint8_t buf;
    int ret;
    ret = tmp421_read(dev, TMP421_REG_MANUFACTURER_ID, &buf, 1);
    if (ret != 0) return -1;
    ret = tmp421_read(dev, TMP421_REG_DEVICE_ID, &buf, 1);
    if (ret != 0) return -1;
    return 0;
}

int tmp421_read_local(struct tmp421_dev *dev, int32_t *temp_local_val)
{
    int ret = tmp421_poll_busy(dev);
    if (ret != 0) return -1;
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_dev *dev, int32_t *temp_remote_val)
{
    int ret = tmp421_poll_busy(dev);
    if (ret != 0) return -1;
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE1_HIGH, TMP421_REG_REMOTE1_LOW, temp_remote_val);
}
