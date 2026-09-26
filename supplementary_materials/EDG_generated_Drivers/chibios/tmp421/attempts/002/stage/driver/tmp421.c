#include "tmp421.h"
#include "hal.h"
#include "hal_i2c.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define TMP421_I2C_ADDR 0x2A
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW  0x10
#define TMP421_REG_REMOTE1_HIGH 0x01
#define TMP421_REG_REMOTE1_LOW  0x11
#define TMP421_REG_STATUS 0x08
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_TIMEOUT_MS 130

static msg_t i2c_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *rxbuf, size_t rxlen) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, &reg, 1, rxbuf, rxlen, TIME_MS2I(TMP421_CONVERSION_TIMEOUT_MS));
    i2cReleaseBus(i2cp);
    return ret;
}

static msg_t i2c_write(struct tmp421_device *dev, uint8_t reg, uint8_t data) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t txbuf[2] = {reg, data};
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 2, NULL, 0, TIME_MS2I(TMP421_CONVERSION_TIMEOUT_MS));
    i2cReleaseBus(i2cp);
    return ret;
}

static int32_t read_temperature(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg) {
    uint8_t high_byte, low_byte;
    if (i2c_write_then_read(dev, high_reg, &high_byte, 1) != MSG_OK) return -1;
    if (i2c_write_then_read(dev, low_reg, &low_byte, 1) != MSG_OK) return -1;
    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    return ((int32_t)raw * 625) / 10;
}

void tmp421_init(struct tmp421_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;
    uint8_t buf;
    i2c_write_then_read(dev, TMP421_REG_MANUFACTURER_ID, &buf, 1);
    i2c_write_then_read(dev, TMP421_REG_DEVICE_ID, &buf, 1);
}

int32_t tmp421_read_local(struct tmp421_device *dev, int32_t *val) {
    uint8_t status;
    if (i2c_write_then_read(dev, TMP421_REG_STATUS, &status, 1) != MSG_OK) return -1;
    if (status & TMP421_BUSY_BIT) return -1;
    int32_t temp = read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW);
    if (temp < 0) return -1;
    *val = temp;
    return 0;
}

int32_t tmp421_read_remote(struct tmp421_device *dev, int32_t *val) {
    uint8_t status;
    if (i2c_write_then_read(dev, TMP421_REG_STATUS, &status, 1) != MSG_OK) return -1;
    if (status & TMP421_BUSY_BIT) return -1;
    int32_t temp = read_temperature(dev, TMP421_REG_REMOTE1_HIGH, TMP421_REG_REMOTE1_LOW);
    if (temp < 0) return -1;
    *val = temp;
    return 0;
}