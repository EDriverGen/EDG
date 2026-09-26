#include "at24c256.h"
#include <rtthread.h>
#include <rtdevice.h>

#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_DELAY_MS 5

int at24c256_init(struct at24c256_device *dev, struct rt_i2c_bus_device *bus)
{
    if (dev == RT_NULL || bus == RT_NULL) {
        return -1;
    }
    dev->bus = bus;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    struct rt_i2c_msg msgs[2];
    uint8_t addr_bytes[2];

    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 2;
    msgs[0].buf = addr_bytes;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2) {
        return -1;
    }
    return 0;
}

int at24c256_write(struct at24c256_device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    size_t offset = 0;
    while (offset < len) {
        size_t page_offset = (addr + offset) % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) {
            chunk = len - offset;
        }

        uint8_t data[2 + chunk];
        data[0] = (uint8_t)((addr + offset) >> 8);
        data[1] = (uint8_t)((addr + offset) & 0xFF);
        for (size_t i = 0; i < chunk; i++) {
            data[2 + i] = buf[offset + i];
        }

        struct rt_i2c_msg msg;
        msg.addr = dev->i2c_addr;
        msg.flags = RT_I2C_WR;
        msg.len = 2 + chunk;
        msg.buf = data;

        if (rt_i2c_transfer(dev->bus, &msg, 1) != 1) {
            return -1;
        }

        rt_thread_mdelay(AT24C256_WRITE_DELAY_MS);

        offset += chunk;
    }
    return 0;
}
