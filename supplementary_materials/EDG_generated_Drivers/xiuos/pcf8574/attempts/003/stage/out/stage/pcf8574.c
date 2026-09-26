#include "pcf8574.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_device *dev, struct I2cBus *bus)
{
    if (!dev || !bus) return -EINVAL;
    dev->bus = bus;
    dev->addr = PCF8574_I2C_ADDR;
    dev->fd = -1;

    // Open I2C device
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -EIO;

    // Set slave address
    uint16_t i2c_addr = dev->addr;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -EIO;
    }

    return 0;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte)
{
    if (!dev || !port_byte) return -EINVAL;
    if (dev->fd < 0) return -EIO;

    // Read one byte from the device (no register pointer)
    uint8_t buf = 0;
    int ret = PrivRead(dev->fd, &buf, 1);
    if (ret < 0) return -EIO;

    *port_byte = buf;
    return 0;
}
