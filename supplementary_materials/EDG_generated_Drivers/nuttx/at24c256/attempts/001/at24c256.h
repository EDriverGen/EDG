#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stddef.h>

struct i2c_master_s;

struct at24c256_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int at24c256_init(struct at24c256_dev_s *dev, struct i2c_master_s *bus);
int at24c256_read(struct at24c256_dev_s *dev, uint16_t addr, uint8_t *buf, size_t len);
int at24c256_write(struct at24c256_dev_s *dev, uint16_t addr, const uint8_t *buf, size_t len);

#endif /* AT24C256_H */