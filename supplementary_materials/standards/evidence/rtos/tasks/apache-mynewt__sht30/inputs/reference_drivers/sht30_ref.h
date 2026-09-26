#ifndef SHT30_APACHE_MYNEWT_REF_H
#define SHT30_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_i2c.h"
#include <stdint.h>

#define SHT30_ADDR_DEFAULT 0x44
#define SHT30_ADDR_ALT     0x45

struct sht30_device {
    uint8_t i2c_num;
    uint16_t addr;
};

int sht30_init(struct sht30_device *dev, uint8_t i2c_num, uint16_t addr);
int sht30_probe(struct sht30_device *dev);
int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent);

#endif
