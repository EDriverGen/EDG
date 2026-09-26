#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>

struct rt_i2c_bus_device;

struct lm75a_device {
    struct rt_i2c_bus_device *bus;
    uint8_t addr;
};

int lm75a_init(struct lm75a_device *dev, struct rt_i2c_bus_device *bus);
int lm75a_read_temperature(struct lm75a_device *dev, int32_t *raw);

#endif /* LM75A_H */