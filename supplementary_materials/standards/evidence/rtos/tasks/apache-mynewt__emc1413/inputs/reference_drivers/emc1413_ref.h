#ifndef EMC1413_APACHE_MYNEWT_REF_H
#define EMC1413_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_i2c.h"
#include <stdint.h>

#define EMC1413_DEFAULT_ADDR         0x4C

#define EMC1413_REG_INTERNAL_TEMP_HI 0x00
#define EMC1413_REG_EXT1_TEMP_HI     0x01
#define EMC1413_REG_CONFIG           0x03
#define EMC1413_REG_EXT1_TEMP_LO     0x10
#define EMC1413_REG_EXT2_TEMP_HI     0x23
#define EMC1413_REG_EXT2_TEMP_LO     0x24
#define EMC1413_REG_INTERNAL_TEMP_LO 0x29
#define EMC1413_REG_MANUFACTURER_ID  0xFE

#define EMC1413_MANUFACTURER_ID      0x5D
#define EMC1413_CONFIG_RANGE         (1U << 2)

enum emc1413_channel {
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};

struct emc1413_device {
    uint8_t i2c_num;
    uint8_t addr;
};

int emc1413_init(struct emc1413_device *dev, uint8_t i2c_num, uint8_t addr);
int emc1413_probe(struct emc1413_device *dev);
int emc1413_read_temperature(struct emc1413_device *dev, enum emc1413_channel channel,
                             int32_t *temp_mcelsius);
int emc1413_set_extended_range(struct emc1413_device *dev, int enable);

#endif
