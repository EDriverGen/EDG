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
    const struct device *bus = dev;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);
    int ret = i2c_write_read(bus, AT24C256_I2C_ADDR, addr_bytes, 2, buf, len);
    if (ret != 0) {
        return -EIO;
    }
    return 0;
}

int at24c256_write(const struct device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    const struct device *bus = dev;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);
    size_t offset = 0;
    while (offset < len) {
        size_t page_offset = (addr + offset) % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) {
            chunk = len - offset;
        }
        uint8_t *write_buf = malloc(2 + chunk);
        if (write_buf == NULL) {
            return -ENOMEM;
        }
        write_buf[0] = (uint8_t)((addr + offset) >> 8);
        write_buf[1] = (uint8_t)((addr + offset) & 0xFF);
        for (size_t i = 0; i < chunk; i++) {
            write_buf[2 + i] = buf[offset + i];
        }
        int ret = i2c_write(bus, write_buf, 2 + chunk, AT24C256_I2C_ADDR);
        free(write_buf);
        if (ret != 0) {
            return -EIO;
        }
        k_msleep(5);
        offset += chunk;
    }
    return 0;
}
