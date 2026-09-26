#include "at24c256.h"
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "nuttx.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64

static int at24c256_write_page(struct at24c256_dev_s *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    struct i2c_msg_s msg[2];
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;

    msg[0].frequency = 100000;
    msg[0].addr = AT24C256_I2C_ADDR;
    msg[0].flags = 0;
    msg[0].buffer = addr_bytes;
    msg[0].length = 2;

    msg[1].frequency = 100000;
    msg[1].addr = AT24C256_I2C_ADDR;
    msg[1].flags = I2C_M_NOSTART;
    msg[1].buffer = (uint8_t *)buf;
    msg[1].length = len;

    int ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return ret;
    }

    up_mdelay(5);
    return 0;
}

int at24c256_init(struct at24c256_dev_s *dev, struct i2c_master_s *bus)
{
    if (!dev || !bus) {
        return -EINVAL;
    }
    dev->bus = bus;
    dev->addr = AT24C256_I2C_ADDR;
    return 0;
}

int at24c256_read(struct at24c256_dev_s *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) {
        return -EINVAL;
    }

    struct i2c_msg_s msg[2];
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;

    msg[0].frequency = 100000;
    msg[0].addr = AT24C256_I2C_ADDR;
    msg[0].flags = 0;
    msg[0].buffer = addr_bytes;
    msg[0].length = 2;

    msg[1].frequency = 100000;
    msg[1].addr = AT24C256_I2C_ADDR;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = buf;
    msg[1].length = len;

    int ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int at24c256_write(struct at24c256_dev_s *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) {
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

        int ret = at24c256_write_page(dev, current_addr, buf + offset, chunk);
        if (ret < 0) {
            return ret;
        }
        offset += chunk;
    }

    return 0;
}
