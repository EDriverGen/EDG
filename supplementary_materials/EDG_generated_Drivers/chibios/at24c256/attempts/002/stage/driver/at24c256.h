#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stddef.h>

struct at24c256_device {
    void *bus_handle;
    uint8_t i2c_addr;
};

void at24c256_init(struct at24c256_device *dev, void *bus_handle);
int at24c256_read(struct at24c256_device *dev, uint16_t addr, uint8_t *buf, size_t len);
int at24c256_write(struct at24c256_device *dev, uint16_t addr, const uint8_t *buf, size_t len);

#endif /* AT24C256_H */