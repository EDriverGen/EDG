#include "pcf8574.h"
#include <stdint.h>
#include <errno.h>
#include <nuttx/i2c/i2c_master.h>

#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_dev_s *dev, struct i2c_master_s *bus)
{
    if (dev == NULL || bus == NULL) {
        return -EINVAL;
    }
    dev->bus = bus;
    dev->addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_dev_s *dev, uint8_t *port_byte)
{
    struct i2c_config_s config;
    int ret;

    if (dev == NULL || dev->bus == NULL || port_byte == NULL) {
        return -EINVAL;
    }

    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;

    ret = I2C_TRANSFER(dev->bus, &config, port_byte, 1);
    if (ret < 0) {
        return ret;
    }

    return 0;
}