#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>

struct pcf8574_device {
    void *bus_handle;
    uint8_t i2c_addr;
};

void pcf8574_init(struct pcf8574_device *dev, void *bus_handle);
void pcf8574_read_port(struct pcf8574_device *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7);

#endif