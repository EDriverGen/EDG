#ifndef __LM75A_REF_H
#define __LM75A_REF_H

#include "i2c_if.h"
#include "osal_time.h"
#include <stdint.h>
#include <stdbool.h>

#define LM75A_DEFAULT_ADDR  0x48
#define LM75A_REG_TEMP      0x00
#define LM75A_REG_CONF      0x01

struct lm75a_device {
    DevHandle bus;
    uint16_t addr;
};

int lm75a_init(struct lm75a_device *dev, DevHandle bus, uint16_t addr);
int lm75a_probe(struct lm75a_device *dev);
int lm75a_read_temperature(struct lm75a_device *dev, int32_t *temp_mcelsius);
int lm75a_read_raw(struct lm75a_device *dev, int16_t *raw_out);

#endif
