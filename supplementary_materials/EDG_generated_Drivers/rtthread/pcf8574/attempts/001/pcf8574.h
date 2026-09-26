#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>
#include <drivers/dev_i2c.h>

struct rt_i2c_bus_device;

struct pcf8574_device {
    struct rt_i2c_bus_device *bus;
    uint8_t addr;
};

int pcf8574_init(struct pcf8574_device *dev, struct rt_i2c_bus_device *bus);
int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte);

#endif /* PCF8574_H */