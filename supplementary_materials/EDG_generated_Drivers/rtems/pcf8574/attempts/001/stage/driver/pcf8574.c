#include "pcf8574.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte)
{
    const char *path = (const char *)dev->bus_handle;
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return -1;
    }

    struct i2c_msg msg;
    uint8_t buf[1];
    msg.addr = dev->i2c_addr;
    msg.flags = I2C_M_RD;
    msg.len = 1;
    msg.buf = buf;

    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;

    int ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        close(fd);
        return -1;
    }

    *port_byte = buf[0];
    close(fd);
    return 0;
}
