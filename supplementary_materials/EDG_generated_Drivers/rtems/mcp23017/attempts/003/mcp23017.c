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
#define MCP23017_I2C_ADDR 0x20
#define IODIRA 0x00
#define IODIRB 0x01
#define GPIOA 0x12
#define GPIOB 0x13

static int i2c_write(int fd, uint8_t addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    msg.addr = addr;
    msg.flags = 0;
    msg.len = len;
    msg.buf = buf;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

static int i2c_read(int fd, uint8_t addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    msg.addr = addr;
    msg.flags = I2C_M_RD;
    msg.len = len;
    msg.buf = buf;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = wlen;
    msgs[0].buf = wbuf;
    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = rlen;
    msgs[1].buf = rbuf;
    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

int mcp23017_init(struct mcp23017_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return -EIO;
    }
    dev->fd = fd;
    dev->addr = MCP23017_I2C_ADDR;

    uint8_t buf[2];
    int ret;

    buf[0] = IODIRA;
    buf[1] = 0x00;
    ret = i2c_write(dev->fd, dev->addr, buf, 2);
    if (ret < 0) {
        close(dev->fd);
        return -EIO;
    }

    buf[0] = IODIRB;
    buf[1] = 0x00;
    ret = i2c_write(dev->fd, dev->addr, buf, 2);
    if (ret < 0) {
        close(dev->fd);
        return -EIO;
    }

    return 0;
}

int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta_byte)
{
    uint8_t cmd = GPIOA;
    int ret = i2c_write_then_read(dev->fd, dev->addr, &cmd, 1, porta_byte, 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb_byte)
{
    uint8_t cmd = GPIOB;
    int ret = i2c_write_then_read(dev->fd, dev->addr, &cmd, 1, portb_byte, 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}
