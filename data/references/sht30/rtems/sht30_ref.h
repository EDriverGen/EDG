#ifndef SHT30_RTEMS_REF_H
#define SHT30_RTEMS_REF_H

#include <stdint.h>
#include <rtems.h>

#define SHT30_ADDR_DEFAULT 0x44
#define SHT30_ADDR_ALT     0x45

struct sht30_device {
    const char *bus_path;
    uint16_t addr;
};

int sht30_init(struct sht30_device *dev, const char *bus_path, uint16_t addr);
int sht30_probe(struct sht30_device *dev);
int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent);

#endif
