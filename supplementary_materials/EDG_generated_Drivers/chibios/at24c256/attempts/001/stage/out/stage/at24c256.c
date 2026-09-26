#include "at24c256.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#include "chibios.h"
#define I2C_TIMEOUT MS2ST(100)

int at24c256_init(at24c256_device_t *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

static int i2c_write_bytes(at24c256_device_t *dev, const uint8_t *txbuf, size_t txlen) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, txlen, NULL, 0, I2C_TIMEOUT);
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

static int i2c_read_bytes(at24c256_device_t *dev, uint8_t *rxbuf, size_t rxlen) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterReceiveTimeout(i2cp, dev->i2c_addr, rxbuf, rxlen, I2C_TIMEOUT);
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

static int i2c_write_then_read(at24c256_device_t *dev, const uint8_t *txbuf, size_t txlen, uint8_t *rxbuf, size_t rxlen) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, txlen, rxbuf, rxlen, I2C_TIMEOUT);
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

int at24c256_read(at24c256_device_t *dev, uint16_t addr, uint8_t *buf, size_t len) {
    if (!dev || !buf || len == 0) return -1;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;
    if (i2c_write_then_read(dev, addr_bytes, 2, buf, len) != 0) {
        return -1;
    }
    return 0;
}

int at24c256_write(at24c256_device_t *dev, uint16_t addr, const uint8_t *buf, size_t len) {
    if (!dev || !buf || len == 0) return -1;
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;
        uint8_t txbuf[2 + chunk];
        txbuf[0] = (current_addr >> 8) & 0xFF;
        txbuf[1] = current_addr & 0xFF;
        memcpy(txbuf + 2, buf + offset, chunk);
        if (i2c_write_bytes(dev, txbuf, 2 + chunk) != 0) {
            return -1;
        }
        offset += chunk;
    }
    return 0;
}
