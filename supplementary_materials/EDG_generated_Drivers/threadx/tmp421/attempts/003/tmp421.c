#include "tmp421.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "threadx.h"
#define TMP421_I2C_ADDR 0x2A
#define TMP421_I2C_ADDR_SHIFTED (TMP421_I2C_ADDR << 1)

#define REG_STATUS 0x08
#define REG_LOCAL_TEMP_HIGH 0x00
#define REG_LOCAL_TEMP_LOW 0x10
#define REG_REMOTE1_TEMP_HIGH 0x01
#define REG_REMOTE1_TEMP_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF

#define BUSY_BIT 0x80

static int tmp421_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle, TMP421_I2C_ADDR_SHIFTED, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_write_only(struct tmp421_device *dev, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, TMP421_I2C_ADDR_SHIFTED, data, 2, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_read_register(struct tmp421_device *dev, uint8_t reg, uint8_t *value)
{
    return tmp421_write_then_read(dev, reg, value, 1);
}

static int tmp421_poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    int ret;
    uint32_t timeout = 130;
    while (timeout > 0) {
        ret = tmp421_read_register(dev, REG_STATUS, &status);
        if (ret != 0) {
            return ret;
        }
        if (!(status & BUSY_BIT)) {
            return 0;
        }
        HAL_Delay(1);
        timeout--;
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int ret;
    int16_t raw;
    int32_t milli;

    ret = tmp421_poll_busy(dev);
    if (ret != 0) {
        return ret;
    }

    ret = tmp421_read_register(dev, high_reg, &high_byte);
    if (ret != 0) {
        return ret;
    }

    ret = tmp421_read_register(dev, low_reg, &low_byte);
    if (ret != 0) {
        return ret;
    }

    raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    milli = ((int32_t)raw * 625) / 10;
    *temp = milli;
    return 0;
}

int tmp421_init(struct tmp421_device *dev, void *bus_handle)
{
    uint8_t id;
    int ret;

    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    ret = tmp421_read_register(dev, REG_MANUFACTURER_ID, &id);
    if (ret != 0 || id != 0x55) {
        return -1;
    }

    ret = tmp421_read_register(dev, REG_DEVICE_ID, &id);
    if (ret != 0 || id != 0x21) {
        return -1;
    }

    return 0;
}

int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, REG_LOCAL_TEMP_HIGH, REG_LOCAL_TEMP_LOW, temp);
}

int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *temp)
{
    return tmp421_read_temperature(dev, REG_REMOTE1_TEMP_HIGH, REG_REMOTE1_TEMP_LOW, temp);
}
