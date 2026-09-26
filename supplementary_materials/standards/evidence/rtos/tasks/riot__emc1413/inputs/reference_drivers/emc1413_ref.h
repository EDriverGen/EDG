/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * EMC1413 Temperature Sensor Driver for RIOT OS
 */
#ifndef __EMC1413_REF_H
#define __EMC1413_REF_H

#include "periph/i2c.h"
#include "ztimer.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {

/* Channel enum for unified read API */
enum emc1413_channel {
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};


#endif

#define EMC1413_ADDR_DEFAULT           0x4C

#define EMC1413_REG_INTERNAL_TEMP      0x00
#define EMC1413_REG_INTERNAL_TEMP_L    0x29
#define EMC1413_REG_EXT1_TEMP_H        0x01
#define EMC1413_REG_EXT1_TEMP_L        0x10
#define EMC1413_REG_EXT2_TEMP_H        0x23
#define EMC1413_REG_EXT2_TEMP_L        0x24
#define EMC1413_REG_STATUS             0x02
#define EMC1413_REG_CONFIG             0x03
#define EMC1413_REG_MFG_ID             0xFE
#define EMC1413_REG_DEV_ID             0xFF

#define EMC1413_MFG_ID_EXPECTED        0x5D

struct emc1413_device
{
    i2c_t bus;          /* RIOT I2C device index */
    uint16_t addr;
};

int emc1413_init(struct emc1413_device *dev, i2c_t bus, uint16_t addr);
int emc1413_probe(struct emc1413_device *dev);
int emc1413_read_internal_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);
int emc1413_read_external1_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);
int emc1413_read_external2_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);

#ifdef __cplusplus
}

/* Channel enum for unified read API */
enum emc1413_channel {
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};


#endif


/* Channel enum for unified read API */
enum emc1413_channel {
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};



/* EVAL_COMPAT_SHIM */
static inline int emc1413_read_temperature(struct emc1413_device *dev,
                                           enum emc1413_channel ch,
                                           int32_t *out) {
    switch (ch) {
    case EMC1413_CH_INTERNAL:  return emc1413_read_internal_temp(dev, out);
    case EMC1413_CH_EXTERNAL_1: return emc1413_read_external1_temp(dev, out);
    case EMC1413_CH_EXTERNAL_2: return emc1413_read_external2_temp(dev, out);
    default: return -1;
    }
}

#endif /* __EMC1413_REF_H */
