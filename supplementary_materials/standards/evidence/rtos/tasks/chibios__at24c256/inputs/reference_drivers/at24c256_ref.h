#ifndef __AT24C256_REF_H
#define __AT24C256_REF_H
#include "hal.h"
#include <stdint.h>

#define AT24C256_ADDR_DEFAULT  0x50
#define AT24C256_PAGE_SIZE     64

struct at24c256_device {
  I2CDriver * bus;
  uint16_t addr;
};

int at24c256_init(struct at24c256_device *dev, I2CDriver * bus, uint16_t addr);
int at24c256_probe(struct at24c256_device *dev);
int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len);
int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len);
#endif
