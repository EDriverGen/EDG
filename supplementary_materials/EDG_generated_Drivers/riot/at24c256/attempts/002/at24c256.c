#include "at24c256.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "riot.h"
#define AT24C256_PAGE_SIZE 64

void at24c256_init(at24c256_t *dev, i2c_t bus, uint8_t addr)
{
    dev->bus = bus;
    dev->addr = addr;
    i2c_acquire(dev->bus);
    i2c_release(dev->bus);
}

int at24c256_read(at24c256_t *dev, uint16_t mem_addr, uint8_t *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return -1;
    }
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(mem_addr >> 8);
    addr_bytes[1] = (uint8_t)(mem_addr & 0xFF);
    
    i2c_acquire(dev->bus);
    int ret = i2c_write_bytes(dev->bus, dev->addr, addr_bytes, 2, 0);
    if (ret != 0) {
        i2c_release(dev->bus);
        return -1;
    }
    ret = i2c_read_bytes(dev->bus, dev->addr, buf, len, 0);
    i2c_release(dev->bus);
    return ret;
}

int at24c256_write(at24c256_t *dev, uint16_t mem_addr, const uint8_t *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return -1;
    }
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = mem_addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) {
            chunk = len - offset;
        }
        uint8_t addr_bytes[2];
        addr_bytes[0] = (uint8_t)(current_addr >> 8);
        addr_bytes[1] = (uint8_t)(current_addr & 0xFF);
        
        i2c_acquire(dev->bus);
        int ret = i2c_write_bytes(dev->bus, dev->addr, addr_bytes, 2, 0);
        if (ret != 0) {
            i2c_release(dev->bus);
            return -1;
        }
        ret = i2c_write_bytes(dev->bus, dev->addr, buf + offset, chunk, 0);
        i2c_release(dev->bus);
        if (ret != 0) {
            return -1;
        }
        ztimer_sleep(ZTIMER_MSEC, 5);
        offset += chunk;
    }
    return 0;
}
