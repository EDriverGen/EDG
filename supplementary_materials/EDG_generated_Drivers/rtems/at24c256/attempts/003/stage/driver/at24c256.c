#include "at24c256.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
int at24c256_init(struct at24c256_device *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -EINVAL;
    dev->fd = (int)(intptr_t)bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

static int i2c_transfer(int fd, uint8_t addr, uint8_t *wbuf, size_t wlen, uint8_t *rbuf, size_t rlen)
{
    struct i2c_msg msgs[2];
    unsigned nmsgs = 0;
    if (wlen > 0) {
        msgs[nmsgs].addr = addr;
        msgs[nmsgs].flags = 0;
        msgs[nmsgs].len = wlen;
        msgs[nmsgs].buf = wbuf;
        nmsgs++;
    }
    if (rlen > 0) {
        msgs[nmsgs].addr = addr;
        msgs[nmsgs].flags = I2C_M_RD;
        msgs[nmsgs].len = rlen;
        msgs[nmsgs].buf = rbuf;
        nmsgs++;
    }
    if (nmsgs == 0) return 0;
    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    int ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) return -EIO;
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    if (addr + len > AT24C256_SIZE) return -EINVAL;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;
    int ret = i2c_transfer(dev->fd, dev->i2c_addr, addr_bytes, 2, buf, len);
    if (ret < 0) return ret;
    return 0;
}

int at24c256_write(struct at24c256_device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    if (addr + len > AT24C256_SIZE) return -EINVAL;
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;
        uint8_t *wb = malloc(2 + chunk);
        if (!wb) return -ENOMEM;
        wb[0] = (current_addr >> 8) & 0xFF;
        wb[1] = current_addr & 0xFF;
        memcpy(wb + 2, buf + offset, chunk);
        int ret = i2c_transfer(dev->fd, dev->i2c_addr, wb, 2 + chunk, NULL, 0);
        free(wb);
        if (ret < 0) return ret;
        usleep(5000);
        offset += chunk;
    }
    return 0;
}
