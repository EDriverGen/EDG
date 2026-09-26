#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stddef.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint8_t addr;
} at24c256_t;

void at24c256_init(at24c256_t *dev, i2c_t bus, uint8_t addr);
int at24c256_read(at24c256_t *dev, uint16_t mem_addr, uint8_t *buf, size_t len);
int at24c256_write(at24c256_t *dev, uint16_t mem_addr, const uint8_t *buf, size_t len);

#endif /* AT24C256_H */
