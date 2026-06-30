#ifndef TMP421_APACHE_MYNEWT_REF_H
#define TMP421_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_i2c.h"
#include <stdint.h>

#define TMP421_DEFAULT_ADDR          0x2A

#define TMP421_REG_LOCAL_TEMP_HI     0x00
#define TMP421_REG_REMOTE_TEMP_HI    0x01
#define TMP421_REG_CONFIG_1          0x09
#define TMP421_REG_CONFIG_1_WR       0x09
#define TMP421_REG_LOCAL_TEMP_LO     0x10
#define TMP421_REG_REMOTE_TEMP_LO    0x11
#define TMP421_REG_MANUFACTURER_ID   0xFE

#define TMP421_MANUFACTURER_ID_TI    0x55
#define TMP421_CONFIG1_RANGE         (1U << 2)

struct tmp421_device {
    uint8_t i2c_num;
    uint8_t addr;
};

int tmp421_init(struct tmp421_device *dev, uint8_t i2c_num, uint8_t addr);
int tmp421_probe(struct tmp421_device *dev);
int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);
int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);
int tmp421_set_extended_range(struct tmp421_device *dev, int enable);

#endif
