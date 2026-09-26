#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "at24c256.h"

#include <zephyr/sys/byteorder.h>
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_SIZE 32768

int at24c256_init(const struct device *dev, const struct device *bus)
{
    (void)dev;
    (void)bus;
    return 0;
}

int at24c256_read(const struct device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return -EINVAL;
    }
    if (addr + len > AT24C256_SIZE) {
        return -EINVAL;
    }
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;
    int ret = i2c_write_read(dev, AT24C256_I2C_ADDR, addr_bytes, 2, buf, len);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int at24c256_write(const struct device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return -EINVAL;
    }
    if (addr + len > AT24C256_SIZE) {
        return -EINVAL;
    }
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) {
            chunk = len - offset;
        }
        uint8_t data[2 + chunk];
        data[0] = (current_addr >> 8) & 0xFF;
        data[1] = current_addr & 0xFF;
        for (size_t i = 0; i < chunk; i++) {
            data[2 + i] = buf[offset + i];
        }
        int ret = i2c_write(dev, data, 2 + chunk, AT24C256_I2C_ADDR);
        if (ret != 0) {
            return ret;
        }
        k_msleep(5);
        offset += chunk;
    }
    return 0;
}
