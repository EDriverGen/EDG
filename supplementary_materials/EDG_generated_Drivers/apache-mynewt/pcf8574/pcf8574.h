#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>

#include <hal/hal_i2c.h>
struct pcf8574_dev {
    uint8_t i2c_num;
    uint8_t addr;
};

int pcf8574_init(struct pcf8574_dev *dev, void *bus_handle);
int pcf8574_read_port(struct pcf8574_dev *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7);

#endif
