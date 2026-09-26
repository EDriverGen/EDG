#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>
#include "i2c_if.h"

#define PCF8574_I2C_ADDR 0x20

#include "openharmony_liteosm.h"
struct pcf8574_device {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int pcf8574_init(struct pcf8574_device *dev, DevHandle bus_handle);
int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7);

#endif
