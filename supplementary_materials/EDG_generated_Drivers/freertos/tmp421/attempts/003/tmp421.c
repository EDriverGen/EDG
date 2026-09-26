#include "tmp421.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#define TMP421_I2C_ADDR 0x2A
#define TMP421_ADDR_8BIT (TMP421_I2C_ADDR << 1)

#define REG_STATUS 0x08
#define REG_LOCAL_HIGH 0x00
#define REG_LOCAL_LOW 0x10
#define REG_REMOTE1_HIGH 0x01
#define REG_REMOTE1_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF

#define BUSY_BIT 0x80

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, TMP421_ADDR_8BIT, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int tmp421_write(struct tmp421_dev *dev, uint8_t reg, uint8_t value)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t data[2] = {reg, value};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, TMP421_ADDR_8BIT, data, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int tmp421_read_byte(struct tmp421_dev *dev, uint8_t reg, uint8_t *value)
{
    return tmp421_write_then_read(dev, reg, value, 1);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int timeout = 130;
    while (timeout--) {
        if (tmp421_read_byte(dev, REG_STATUS, &status) != 0)
            return -1;
        if (!(status & BUSY_BIT))
            return 0;
        HAL_Delay(1);
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
    uint8_t high_byte, low_byte;
    if (tmp421_poll_busy(dev) != 0)
        return -1;
    if (tmp421_read_byte(dev, high_reg, &high_byte) != 0)
        return -1;
    if (tmp421_read_byte(dev, low_reg, &low_byte) != 0)
        return -1;
    int16_t high_signed = (int16_t)(int8_t)high_byte;
    uint8_t low_nibble = low_byte >> 4;
    int16_t raw = (high_signed << 4) | low_nibble;
    *temp_milli = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;
    uint8_t id;
    if (tmp421_read_byte(dev, REG_MANUFACTURER_ID, &id) != 0)
        return -1;
    if (id != 0x55)
        return -1;
    if (tmp421_read_byte(dev, REG_DEVICE_ID, &id) != 0)
        return -1;
    if (id != 0x21)
        return -1;
    return 0;
}

int tmp421_read_local(struct tmp421_dev *dev, int32_t *temp_local_val)
{
    return tmp421_read_temperature(dev, REG_LOCAL_HIGH, REG_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_dev *dev, int32_t *temp_remote_val)
{
    return tmp421_read_temperature(dev, REG_REMOTE1_HIGH, REG_REMOTE1_LOW, temp_remote_val);
}
