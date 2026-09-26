#ifndef __TMP105_REF_H
#define __TMP105_REF_H

#include "i2c_if.h"
#include "osal_time.h"
#include <stdint.h>

#define TMP105_ADDR_DEFAULT  0x48
#define TMP105_REG_TEMP      0x00
#define TMP105_REG_CONF      0x01

struct tmp105_device {
    DevHandle bus;
    uint16_t addr;
};

int tmp105_init(struct tmp105_device *dev, DevHandle bus, uint16_t addr);
int tmp105_probe(struct tmp105_device *dev);
int tmp105_read_temperature(struct tmp105_device *dev, int32_t *temp_mcelsius);

#endif
