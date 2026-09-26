#include "pcf8574.h"
#include "rtems.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    dev->fd = open(path, O_RDWR);
    if (dev->fd < 0) {
        return -1;
    }
    dev->addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t buf[1];
    int ret;

    msg.addr = dev->addr;
    msg.flags = I2C_M_RD;
    msg.len = 1;
    msg.buf = buf;

    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;

    ret = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        close(dev->fd);
        return -1;
    }

    *port_byte = buf[0];
    return 0;
}
