#include "pcf8574.h"
#include <stdint.h>
#include <stddef.h>

#include <drivers/dev_i2c.h>
#include "rtthread.h"
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_device *dev, struct rt_i2c_bus_device *bus)
{
    if (dev == NULL || bus == NULL) {
        return -1;
    }
    dev->bus = bus;
    dev->addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[1];
    int ret;

    if (dev == NULL || dev->bus == NULL || port_byte == NULL) {
        return -1;
    }

    msgs[0].addr = dev->addr;
    msgs[0].flags = 1; /* read */
    msgs[0].len = 1;
    msgs[0].buf = buf;

    ret = rt_i2c_transfer(dev->bus, msgs, 1);
    if (ret != 1) {
        return -1;
    }

    *port_byte = buf[0];
    return 0;
}
