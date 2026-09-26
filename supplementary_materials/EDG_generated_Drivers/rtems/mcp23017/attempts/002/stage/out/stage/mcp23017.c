#include "mcp23017.h"
#include "rtems.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
static int i2c_transfer(int fd, uint8_t addr, uint8_t *wbuf, size_t wlen, uint8_t *rbuf, size_t rlen)
{
    struct i2c_msg msgs[2];
    int nmsgs = 0;

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

    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
        return -EIO;
    }
    return 0;
}

int mcp23017_init(struct mcp23017_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return -EIO;
    }
    dev->fd = fd;
    dev->i2c_addr = MCP23017_I2C_ADDR;

    uint8_t wbuf[2];
    int ret;

    // IODIRA = 0x00
    wbuf[0] = MCP23017_IODIRA;
    wbuf[1] = 0x00;
    ret = i2c_transfer(dev->fd, dev->i2c_addr, wbuf, 2, NULL, 0);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    // IODIRB = 0x00
    wbuf[0] = MCP23017_IODIRB;
    wbuf[1] = 0x00;
    ret = i2c_transfer(dev->fd, dev->i2c_addr, wbuf, 2, NULL, 0);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    return 0;
}

int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta_byte)
{
    uint8_t wbuf[1];
    wbuf[0] = MCP23017_GPIOA;
    int ret = i2c_transfer(dev->fd, dev->i2c_addr, wbuf, 1, porta_byte, 1);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb_byte)
{
    uint8_t wbuf[1];
    wbuf[0] = MCP23017_GPIOB;
    int ret = i2c_transfer(dev->fd, dev->i2c_addr, wbuf, 1, portb_byte, 1);
    if (ret < 0) {
        return ret;
    }
    return 0;
}
