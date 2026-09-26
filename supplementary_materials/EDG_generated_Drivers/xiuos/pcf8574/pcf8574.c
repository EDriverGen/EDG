#include "pcf8574.h"
#include "transform.h"
#include "bus.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bus_i2c.h"
#include "dev_i2c.h"
#include "bus_pin.h"
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_device *dev, struct I2cBus *bus)
{
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = PCF8574_I2C_ADDR;

    dev->bus = bus;
    dev->i2c_addr = PCF8574_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) {
        return -1;
    }

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret != 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -1;
    }

    return 0;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte)
{
    uint8_t buf[1];
    int ret;

    if (dev->fd < 0) {
        return -1;
    }

    ret = PrivRead(dev->fd, buf, 1);
    if (ret != 1) {
        return -1;
    }

    *port_byte = buf[0];
    return 0;
}
