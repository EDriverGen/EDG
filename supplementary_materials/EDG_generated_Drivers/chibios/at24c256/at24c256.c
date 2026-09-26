#include "at24c256.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#include "chibios.h"
#define TIMEOUT_MS 100
#define MS2ST(ms) TIME_MS2I(ms)

int at24c256_init(struct at24c256_device *dev, I2CDriver *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

static int at24c256_wait_write_cycle(struct at24c256_device *dev) {
    uint8_t dummy;
    msg_t ret;
    sysinterval_t timeout = MS2ST(5);
    for (int i = 0; i < 10; i++) {
        i2cAcquireBus(dev->bus_handle);
        ret = i2cMasterTransmitTimeout(dev->bus_handle, dev->i2c_addr, NULL, 0, &dummy, 0, timeout);
        i2cReleaseBus(dev->bus_handle);
        if (ret == MSG_OK) return 0;
        chThdSleepMilliseconds(1);
    }
    return -1;
}

int at24c256_write(struct at24c256_device *dev, uint16_t addr, const uint8_t *buf, size_t len) {
    if (!dev || !buf || len == 0) return -1;
    if (addr + len > AT24C256_SIZE) return -1;
    
    size_t offset = 0;
    while (offset < len) {
        size_t page_offset = (addr + offset) % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;
        
        uint8_t txbuf[2 + chunk];
        txbuf[0] = (addr + offset) >> 8;
        txbuf[1] = (addr + offset) & 0xFF;
        memcpy(&txbuf[2], buf + offset, chunk);
        
        i2cAcquireBus(dev->bus_handle);
        msg_t ret = i2cMasterTransmitTimeout(dev->bus_handle, dev->i2c_addr, txbuf, 2 + chunk, NULL, 0, MS2ST(TIMEOUT_MS));
        i2cReleaseBus(dev->bus_handle);
        if (ret != MSG_OK) return -1;
        
        if (at24c256_wait_write_cycle(dev) != 0) return -1;
        
        offset += chunk;
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t addr, uint8_t *buf, size_t len) {
    if (!dev || !buf || len == 0) return -1;
    if (addr + len > AT24C256_SIZE) return -1;
    
    uint8_t addr_bytes[2];
    addr_bytes[0] = addr >> 8;
    addr_bytes[1] = addr & 0xFF;
    
    i2cAcquireBus(dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout(dev->bus_handle, dev->i2c_addr, addr_bytes, 2, buf, len, MS2ST(TIMEOUT_MS));
    i2cReleaseBus(dev->bus_handle);
    
    if (ret != MSG_OK) return -1;
    return 0;
}
