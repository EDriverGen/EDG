#ifndef __SHT30_REF_H
#define __SHT30_REF_H
#include "tx_api.h"
#include <stdint.h>

#define SHT30_ADDR_DEFAULT  0x44
#define SHT30_ADDR_ALT      0x45


struct sht30_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct sht30_device {
  void *bus_context;
  const struct sht30_i2c_ops *ops;
  uint16_t addr;
};

int sht30_init(struct sht30_device *dev, void *bus_context, const struct sht30_i2c_ops *ops, uint16_t addr);
int sht30_probe(struct sht30_device *dev);
int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent);
#endif
