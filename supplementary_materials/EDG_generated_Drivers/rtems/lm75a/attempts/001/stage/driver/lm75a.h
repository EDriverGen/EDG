#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>
#include <stddef.h>

#include <dev/i2c/i2c.h>
#include <linux/i2c-dev.h>
#include "rtems.h"
#include "fcntl.h"
#include <sys/ioctl.h>
#include "unistd.h"
struct i2c_msg;

typedef struct lm75a_dev {
    void *bus_handle;
    uint8_t i2c_addr;
} lm75a_dev_t;

int lm75a_init(lm75a_dev_t *dev, void *bus_handle);
int lm75a_read_temperature(lm75a_dev_t *dev, int32_t *raw);
int lm75a_read_temperature_mdegc(lm75a_dev_t *dev, int32_t *temperature_mdegC);

#endif
