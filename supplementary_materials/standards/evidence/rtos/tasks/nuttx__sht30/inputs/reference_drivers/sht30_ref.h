#ifndef __SHT30_REF_H
#define __SHT30_REF_H
#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#define SHT30_ADDR_DEFAULT  0x44
#define SHT30_ADDR_ALT      0x45

struct sht30_device {
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
};

int sht30_init(FAR struct sht30_device *dev, FAR struct i2c_master_s *i2c, uint16_t addr);
int sht30_probe(struct sht30_device *dev);
int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent);
#endif
