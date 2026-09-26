#ifndef __SHT30_REF_H
#define __SHT30_REF_H
#include "i2c_if.h"
#include "osal_time.h"
#include <stdint.h>

#define SHT30_ADDR_DEFAULT  0x44
#define SHT30_ADDR_ALT      0x45

struct sht30_device {
    DevHandle bus;
    uint16_t addr;
};

int sht30_init(struct sht30_device *dev, DevHandle bus, uint16_t addr);
int sht30_probe(struct sht30_device *dev);
int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent);
#endif
