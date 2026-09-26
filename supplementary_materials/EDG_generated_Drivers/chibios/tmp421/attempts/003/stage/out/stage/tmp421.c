#include "tmp421.h"
#include "hal.h"
#include "hal_i2c.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define TMP421_I2C_ADDR 0x2A
#define TMP421_STATUS_REG 0x08
#define TMP421_LOCAL_HIGH 0x00
#define TMP421_LOCAL_LOW 0x10
#define TMP421_REMOTE_HIGH 0x01
#define TMP421_REMOTE_LOW 0x11
#define TMP421_MANUFACTURER_ID 0xFE
#define TMP421_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_TIMEOUT_MS 130

static msg_t i2c_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *rxbuf, size_t rxlen) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, &reg, 1, rxbuf, rxlen, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
    return ret;
}

static msg_t i2c_write(struct tmp421_device *dev, uint8_t reg, uint8_t data) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t txbuf[2] = {reg, data};
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
    return ret;
}

static int poll_busy(struct tmp421_device *dev) {
    uint8_t status;
    for (int i = 0; i < TMP421_CONVERSION_TIMEOUT_MS; i++) {
        if (i2c_write_then_read(dev, TMP421_STATUS_REG, &status, 1) != MSG_OK) {
            return -1;
        }
        if (!(status & TMP421_BUSY_BIT)) {
            return 0;
        }
        chThdSleepMilliseconds(1);
    }
    return -1;
}

void tmp421_init(struct tmp421_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;
    uint8_t buf;
    i2c_write_then_read(dev, TMP421_MANUFACTURER_ID, &buf, 1);
    i2c_write_then_read(dev, TMP421_DEVICE_ID, &buf, 1);
}

int32_t tmp421_read_local(struct tmp421_device *dev, int32_t *out) {
    if (poll_busy(dev) != 0) {
        return -1;
    }
    uint8_t high, low;
    if (i2c_write_then_read(dev, TMP421_LOCAL_HIGH, &high, 1) != MSG_OK) {
        return -1;
    }
    if (i2c_write_then_read(dev, TMP421_LOCAL_LOW, &low, 1) != MSG_OK) {
        return -1;
    }
    int16_t raw = ((int16_t)((uint16_t)high << 8) | low);
    raw >>= 4;
    if (raw & 0x0800) {
        raw |= 0xF000;
    }
    *out = ((int32_t)raw * 625) / 10;
    return 0;
}

int32_t tmp421_read_remote(struct tmp421_device *dev, int32_t *out) {
    if (poll_busy(dev) != 0) {
        return -1;
    }
    uint8_t high, low;
    if (i2c_write_then_read(dev, TMP421_REMOTE_HIGH, &high, 1) != MSG_OK) {
        return -1;
    }
    if (i2c_write_then_read(dev, TMP421_REMOTE_LOW, &low, 1) != MSG_OK) {
        return -1;
    }
    int16_t raw = ((int16_t)((uint16_t)high << 8) | low);
    raw >>= 4;
    if (raw & 0x0800) {
        raw |= 0xF000;
    }
    *out = ((int32_t)raw * 625) / 10;
    return 0;
}