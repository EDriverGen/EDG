#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

struct i2c_master_s;

struct pcf8574_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int pcf8574_init(struct pcf8574_dev_s *dev, struct i2c_master_s *bus);
int pcf8574_read_port(struct pcf8574_dev_s *dev, uint8_t *port_byte);

#endif /* PCF8574_H */