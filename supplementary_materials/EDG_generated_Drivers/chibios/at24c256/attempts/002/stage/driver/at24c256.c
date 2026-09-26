#include "at24c256.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#include "chibios.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_CYCLE_MS 5

static msg_t i2c_write(I2CDriver *i2cp, i2caddr_t addr, const uint8_t *txbuf, size_t txbytes)
{
    return i2cMasterTransmitTimeout(i2cp, addr, txbuf, txbytes, NULL, 0, TIME_MS2I(100));
}

static msg_t i2c_read(I2CDriver *i2cp, i2caddr_t addr, uint8_t *rxbuf, size_t rxbytes)
{
    return i2cMasterReceiveTimeout(i2cp, addr, rxbuf, rxbytes, TIME_MS2I(100));
}

void at24c256_init(struct at24c256_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
}

int at24c256_read(struct at24c256_device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);

    i2cAcquireBus(i2cp);
    msg_t res = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, addr_bytes, 2, buf, len, TIME_MS2I(100));
    i2cReleaseBus(i2cp);

    return (res == MSG_OK) ? 0 : -1;
}

int at24c256_write(struct at24c256_device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    size_t offset = 0;
    while (offset < len) {
        size_t page_offset = (addr + offset) % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;

        uint8_t txbuf[2 + chunk];
        uint16_t cur_addr = addr + offset;
        txbuf[0] = (uint8_t)(cur_addr >> 8);
        txbuf[1] = (uint8_t)(cur_addr & 0xFF);
        memcpy(&txbuf[2], buf + offset, chunk);

        i2cAcquireBus(i2cp);
        msg_t res = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 2 + chunk, NULL, 0, TIME_MS2I(100));
        i2cReleaseBus(i2cp);

        if (res != MSG_OK) return -1;

        chThdSleepMilliseconds(AT24C256_WRITE_CYCLE_MS);
        offset += chunk;
    }
    return 0;
}
