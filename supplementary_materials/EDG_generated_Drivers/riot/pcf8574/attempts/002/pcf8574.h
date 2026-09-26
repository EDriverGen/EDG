#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>
#include <stddef.h>
#include "periph/i2c.h"

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} pcf8574_t;

int pcf8574_init(pcf8574_t *dev, i2c_t bus, uint8_t addr);
int pcf8574_read_port(pcf8574_t *dev, uint8_t *port_byte);

#endif /* PCF8574_H */
