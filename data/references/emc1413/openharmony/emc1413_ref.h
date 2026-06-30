#ifndef __EMC1413_REF_H
#define __EMC1413_REF_H

#include "i2c_if.h"
#include "osal_time.h"
#include <stdint.h>

#define EMC1413_ADDR_DEFAULT       0x4C
#define EMC1413_REG_INTERNAL_TEMP  0x00
#define EMC1413_REG_INTERNAL_TEMP_L 0x29
#define EMC1413_REG_EXT1_TEMP_H   0x01
#define EMC1413_REG_EXT1_TEMP_L   0x10
#define EMC1413_REG_EXT2_TEMP_H   0x23
#define EMC1413_REG_EXT2_TEMP_L   0x24
#define EMC1413_REG_MFG_ID        0xFE
#define EMC1413_REG_PRODUCT_ID    0xFD
#define EMC1413_MFG_ID_EXPECTED   0x5D
#define EMC1413_PRODUCT_ID_EXPECTED 0x21

struct emc1413_device {
    DevHandle bus;
    uint16_t addr;
};

int emc1413_init(struct emc1413_device *dev, DevHandle bus, uint16_t addr);
int emc1413_probe(struct emc1413_device *dev);
int emc1413_read_internal_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);
int emc1413_read_external1_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);
int emc1413_read_external2_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);


/* Channel enum for unified read API */
enum emc1413_channel {
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};

int emc1413_read_temperature(struct emc1413_device *dev, enum emc1413_channel channel, int32_t *temp_mcelsius);

#endif
