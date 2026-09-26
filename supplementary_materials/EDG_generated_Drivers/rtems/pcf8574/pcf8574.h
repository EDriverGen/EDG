#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>

struct pcf8574_device {
    void *bus_handle;
    uint8_t i2c_addr;
};

int pcf8574_init(struct pcf8574_device *dev, void *bus_handle);
int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *port_byte);

#endif /* PCF8574_H */