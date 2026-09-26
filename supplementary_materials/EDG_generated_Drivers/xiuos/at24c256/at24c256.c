#include "at24c256.h"
#include "transform.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "bus.h"
#include "bus_pin.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64

int at24c256_init(struct at24c256_dev *dev, void *bus_handle)
{
    (void)bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) {
        return -1;
    }
    uint16_t i2c_addr = dev->i2c_addr;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -1;
    }
    return 0;
}

static int i2c_write_raw(int fd, const uint8_t *buf, size_t len)
{
    return PrivWrite(fd, buf, len);
}

static int i2c_read_raw(int fd, uint8_t *buf, size_t len)
{
    return PrivRead(fd, buf, len);
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (dev->fd < 0) return -1;
    if (len == 0) return 0;
    // Dummy write to set address (random read)
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;
    int ret = i2c_write_raw(dev->fd, addr_bytes, 2);
    if (ret < 0) return -1;
    // Wait for write cycle (minimal delay, but for read no write cycle)
    PrivTaskDelay(1);
    // Sequential read
    ret = i2c_read_raw(dev->fd, buf, len);
    if (ret < 0) return -1;
    return (int)len;
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (dev->fd < 0) return -1;
    if (len == 0) return 0;
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;
        // Build write buffer: address bytes + data
        uint8_t wbuf[2 + chunk];
        wbuf[0] = (current_addr >> 8) & 0xFF;
        wbuf[1] = current_addr & 0xFF;
        memcpy(wbuf + 2, buf + offset, chunk);
        int ret = i2c_write_raw(dev->fd, wbuf, 2 + chunk);
        if (ret < 0) return -1;
        // Wait for write cycle
        PrivTaskDelay(5);
        offset += chunk;
    }
    return (int)len;
}
