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
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr) << 1;
    if (HAL_I2C_Mem_Read(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_write_only(struct tmp421_dev *dev, uint8_t reg, uint8_t value)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr) << 1;
    uint8_t data[2] = {reg, value};
    if (HAL_I2C_Master_Transmit(hi2c, addr, data, 2, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

static int tmp421_read_register(struct tmp421_dev *dev, uint8_t reg, uint8_t *value)
{
    return tmp421_write_then_read(dev, reg, value, 1);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int timeout = 200;
    while (timeout--) {
        if (tmp421_read_register(dev, TMP421_REG_STATUS, &status) != 0) {
            return -1;
        }
        if (!(status & 0x80)) {
            return 0;
        }
        HAL_Delay(1);
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
    uint8_t high_byte, low_byte;
    if (tmp421_poll_busy(dev) != 0) {
        return -1;
    }
    if (tmp421_read_register(dev, high_reg, &high_byte) != 0) {
        return -1;
    }
    if (tmp421_read_register(dev, low_reg, &low_byte) != 0) {
        return -1;
    }
    int16_t high_signed = (int16_t)(int8_t)high_byte;
    uint8_t low_nibble = (low_byte >> 4) & 0x0F;
    int32_t combined = ((int32_t)high_signed << 4) | low_nibble;
    *temp_milli = (combined * 625) / 10;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;
    uint8_t id;
    if (tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &id) != 0) {
        return -1;
    }
    if (id != TMP421_MANUFACTURER_ID_EXPECTED) {
        return -1;
    }
    if (tmp421_read_register(dev, TMP421_REG_DEVICE_ID, &id) != 0) {
        return -1;
    }
    if (id != TMP421_DEVICE_ID_EXPECTED) {
        return -1;
    }
    HAL_Delay(TMP421_CONVERSION_DELAY_MS);
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
