#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stddef.h>

#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_SIZE 32768

struct at24c256_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int at24c256_init(struct at24c256_dev *dev, void *bus_handle);
int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len);
int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len);

#endif /* AT24C256_H */