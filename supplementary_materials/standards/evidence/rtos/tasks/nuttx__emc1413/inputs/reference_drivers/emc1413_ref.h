/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 3-Channel Temperature Sensor Driver for NuttX
 */
#ifndef __EMC1413_REF_H
#define __EMC1413_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{

/* Channel enum for unified read API */
enum emc1413_channel {
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};


#endif

#define EMC1413_ADDR_DEFAULT         0x4C
#define EMC1413_I2C_FREQ             100000

/* Register Map */
#define EMC1413_REG_INTERNAL_TEMP_HI 0x00
#define EMC1413_REG_INTERNAL_TEMP_LO 0x29
#define EMC1413_REG_EXT1_TEMP_HI     0x01
#define EMC1413_REG_EXT1_TEMP_LO     0x10
#define EMC1413_REG_EXT2_TEMP_HI     0x23
#define EMC1413_REG_EXT2_TEMP_LO     0x24
#define EMC1413_REG_STATUS           0x02
#define EMC1413_REG_CONFIG           0x03
#define EMC1413_REG_CONV_RATE        0x04
#define EMC1413_REG_MANUFACTURER_ID  0xFE
#define EMC1413_REG_PRODUCT_ID       0xFD

/* Expected ID values */
#define EMC1413_MANUFACTURER_SMSC    0x5D
#define EMC1413_PRODUCT_ID           0x21

struct emc1413_device
{
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
};

int emc1413_init(FAR struct emc1413_device *dev,
                 FAR struct i2c_master_s *i2c,
                 uint8_t addr);
int emc1413_probe(FAR struct emc1413_device *dev);

int emc1413_read_internal_temp(FAR struct emc1413_device *dev,
                               FAR int32_t *temp_mcelsius);
int emc1413_read_ext1_temp(FAR struct emc1413_device *dev,
                           FAR int32_t *temp_mcelsius);
int emc1413_read_ext2_temp(FAR struct emc1413_device *dev,
                           FAR int32_t *temp_mcelsius);

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
    case EMC1413_CH_EXTERNAL_1: return emc1413_read_ext1_temp(dev, out);
    case EMC1413_CH_EXTERNAL_2: return emc1413_read_ext2_temp(dev, out);
    default: return -1;
    }
}

#endif /* __EMC1413_REF_H */
