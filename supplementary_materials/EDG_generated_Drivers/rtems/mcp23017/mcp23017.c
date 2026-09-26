#include "mcp23017.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define MCP23017_REG_IODIRA 0x00
#define MCP23017_REG_IODIRB 0x01
#define MCP23017_REG_GPIOA 0x12
#define MCP23017_REG_GPIOB 0x13

static int i2c_write_read(struct mcp23017_device *dev, uint8_t reg, uint8_t *rxbuf, uint16_t rxlen)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = rxlen;
    msgs[1].buf = rxbuf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    ret = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int i2c_write(struct mcp23017_device *dev, uint8_t *data, uint16_t len)
{
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len = len;
    msgs[0].buf = data;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 1;

    ret = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int mcp23017_init(struct mcp23017_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    uint8_t buf[2];
    int ret;

    dev->i2c_addr = MCP23017_I2C_ADDR;
    dev->fd = open(path, O_RDWR);
    if (dev->fd < 0) {
        return -EIO;
    }

    buf[0] = MCP23017_REG_IODIRA;
    buf[1] = 0x00;
    ret = i2c_write(dev, buf, 2);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    buf[0] = MCP23017_REG_IODIRB;
    buf[1] = 0x00;
    ret = i2c_write(dev, buf, 2);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    return 0;
}

int mcp23017_read_porta(struct mcp23017_device *dev, uint8_t *porta_byte)
{
    uint8_t reg = MCP23017_REG_GPIOA;
    uint8_t rx;
    int ret;

    ret = i2c_write_read(dev, reg, &rx, 1);
    if (ret < 0) {
        return ret;
    }
    *porta_byte = rx;
    return 0;
}

int mcp23017_read_portb(struct mcp23017_device *dev, uint8_t *portb_byte)
{
    uint8_t reg = MCP23017_REG_GPIOB;
    uint8_t rx;
    int ret;

    ret = i2c_write_read(dev, reg, &rx, 1);
    if (ret < 0) {
        return ret;
    }
    *portb_byte = rx;
    return 0;
}
