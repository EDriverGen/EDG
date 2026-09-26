#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>

struct lm75a_device {
    int fd;
    uint8_t i2c_addr;
};

int lm75a_init(struct lm75a_device *dev, void *bus_handle);
int lm75a_read_temp(struct lm75a_device *dev, int32_t *raw);

#endif