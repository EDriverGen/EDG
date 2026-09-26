#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stddef.h>

#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_SIZE 32768

struct device;

int at24c256_init(const struct device *dev, const struct device *bus);
int at24c256_read(const struct device *dev, uint16_t addr, uint8_t *buf, size_t len);
int at24c256_write(const struct device *dev, uint16_t addr, const uint8_t *buf, size_t len);

#endif /* AT24C256_H */