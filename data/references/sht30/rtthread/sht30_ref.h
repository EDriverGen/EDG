#ifndef __SHT30_REF_H
#define __SHT30_REF_H
#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

#define SHT30_ADDR_DEFAULT  0x44
#define SHT30_ADDR_ALT      0x45

struct sht30_device {
  struct rt_i2c_bus_device * bus;
  uint16_t addr;
};

int sht30_init(struct sht30_device *dev, struct rt_i2c_bus_device * bus, uint16_t addr);
int sht30_probe(struct sht30_device *dev);
int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent);
#endif
