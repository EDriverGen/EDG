#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stddef.h>

#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_SIZE 32768

typedef struct {
    void *bus_handle;
    uint8_t i2c_addr;
} at24c256_device_t;

int at24c256_init(at24c256_device_t *dev, void *bus_handle);
int at24c256_read(at24c256_device_t *dev, uint16_t addr, uint8_t *buf, size_t len);
int at24c256_write(at24c256_device_t *dev, uint16_t addr, const uint8_t *buf, size_t len);

#endif