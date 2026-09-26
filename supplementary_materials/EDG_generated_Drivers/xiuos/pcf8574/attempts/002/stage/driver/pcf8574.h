#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>
#include "bus_i2c.h"

#define PCF8574_I2C_ADDR 0x20

struct I2cBus;

struct pcf8574_device {
    struct I2cBus *bus;
    int fd;
    uint8_t i2c_addr;
};

int pcf8574_init(struct pcf8574_device *dev, struct I2cBus *bus);
int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte);

#endif