#ifndef __SHT30_REF_H
#define __SHT30_REF_H
#include <transform.h>
#include <stdint.h>

#define SHT30_ADDR_DEFAULT  0x44
#define SHT30_ADDR_ALT      0x45

struct sht30_device {
  int fd;
  uint16_t addr;
};

int sht30_init(struct sht30_device *dev, const char *i2c_path, uint16_t addr);
int sht30_probe(struct sht30_device *dev);
int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent);
#endif
