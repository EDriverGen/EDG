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
    if (!dev || !bus_handle) return -EINVAL;
    const char *path = (const char *)bus_handle;
    int fd = PrivOpen(path, 0);
    if (fd < 0) return fd;
    uint16_t i2c_addr = AT24C256_I2C_ADDR;
    struct PrivIoctlCfg cfg;
    cfg.ioctl_driver_type = I2C_TYPE;
    cfg.args = &i2c_addr;
    int ret = PrivIoctl(fd, OPE_INT, &cfg);
    if (ret < 0) {
        PrivClose(fd);
        return ret;
    }
    dev->fd = fd;
    dev->i2c_addr = i2c_addr;
    return 0;
}

static int i2c_write_raw(int fd, const uint8_t *data, size_t len)
{
    return PrivWrite(fd, data, len);
}

static int i2c_read_raw(int fd, uint8_t *buf, size_t len)
{
    return PrivRead(fd, buf, len);
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    int fd = dev->fd;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;
    int ret = i2c_write_raw(fd, addr_bytes, 2);
    if (ret < 0) return ret;
    return i2c_read_raw(fd, buf, len);
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    int fd = dev->fd;
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;
        uint8_t *out = malloc(2 + chunk);
        if (!out) return -ENOMEM;
        out[0] = (current_addr >> 8) & 0xFF;
        out[1] = current_addr & 0xFF;
        memcpy(out + 2, buf + offset, chunk);
        int ret = i2c_write_raw(fd, out, 2 + chunk);
        free(out);
        if (ret < 0) return ret;
        PrivTaskDelay(5);
        offset += chunk;
    }
    return (int)len;
}
