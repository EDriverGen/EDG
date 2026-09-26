#include "at24c256.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64

int at24c256_init(struct at24c256_device *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -1;
    dev->fd = (int)(intptr_t)bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

static int i2c_transfer(int fd, struct i2c_msg *msgs, int nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

int at24c256_read(struct at24c256_device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -1;
    int fd = dev->fd;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;
    struct i2c_msg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = addr_bytes;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    if (i2c_transfer(fd, msgs, 2) != 2) return -1;
    return 0;
}

int at24c256_write(struct at24c256_device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -1;
    int fd = dev->fd;
    size_t offset = 0;
    while (offset < len) {
        size_t page_offset = addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;
        uint8_t data[2 + chunk];
        data[0] = ((addr + offset) >> 8) & 0xFF;
        data[1] = (addr + offset) & 0xFF;
        for (size_t i = 0; i < chunk; i++) data[2 + i] = buf[offset + i];
        struct i2c_msg msg;
        msg.addr = dev->i2c_addr;
        msg.flags = 0;
        msg.len = 2 + chunk;
        msg.buf = data;
        if (i2c_transfer(fd, &msg, 1) != 1) return -1;
        usleep(5000);
        offset += chunk;
    }
    return 0;
}
