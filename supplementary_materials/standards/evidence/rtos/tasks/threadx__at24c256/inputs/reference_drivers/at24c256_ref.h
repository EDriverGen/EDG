#ifndef __AT24C256_REF_H
#define __AT24C256_REF_H
#include "tx_api.h"
#include <stdint.h>

#define AT24C256_ADDR_DEFAULT  0x50
#define AT24C256_PAGE_SIZE     64


struct at24c256_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct at24c256_device {
  void *bus_context;
  const struct at24c256_i2c_ops *ops;
  uint16_t addr;
};

int at24c256_init(struct at24c256_device *dev, void *bus_context, const struct at24c256_i2c_ops *ops, uint16_t addr);
int at24c256_probe(struct at24c256_device *dev);
int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len);
int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len);
#endif
